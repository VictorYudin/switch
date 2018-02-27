in highp vec2 uv;

uniform highp vec3 gLightPos;
uniform highp vec3 gE;
uniform sampler2D gSamplerP;
uniform sampler2D gSamplerN;
uniform sampler2D gSamplerC;
uniform sampler2D gSamplerEnv;
uniform highp mat4 gMVP;

layout(location = 0) out highp vec4 Ci;

#define M_PI 3.141592653589793

/**
 * - Follows closely the following paper:
 *
 * [1] "GPU-Based Importance Sampling"
 * Mark Colbert, Jaroslav Kivánek
 * https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
 *
 * [2] "Artist Friendly Metalic Fresnel"
 * Ole Gulbrandsen Framestore
 *
 * [3] "Microfacet Models for Refraction through Rough Surfaces"
 */

/**
 * @brief Equation 12 of [2].
 */
highp float gulbrandsenMapping_getn(highp float r, highp float g)
{
    highp float sqrtr = sqrt(r);
    highp float nMin = (1.0 - r) / (1.0 + r);
    highp float nMax = (1.0 + sqrtr) / (1.0 - sqrtr);

    return nMin * g + (1.0 - g) * nMax;
}

/**
 * @brief Equation 2 of [2].
 */
highp float gulbrandsenMapping_getk2(highp float r, highp float n)
{
    highp float nplusone = n + 1.0;
    highp float nminusone = n - 1.0;
    highp float nr = nplusone * nplusone * r - nminusone * nminusone;
    return nr / (1.0 - r);
}

/**
 * @brief Remaps user-friendly values to the IOR and complex IOR. Follows
 * closely to [2]. Light reﬂecting off metallic surfaces is described by the
 * Fresnel equations [Born and Wolf 1999], which are controlled by the complex
 * index of refraction eta = n + iota * k. Together, n and k determine the two
 * characteristics of the Fresnel curve for a material.
 *
 * @param r is the reflectance.
 * @param g is the edge tint.
 *
 * @return n and k2
 */
highp vec2 gulbrandsenMapping(highp float r, highp float g)
{
    highp float rr = clamp(r, 0.0, 1.0 - 0.001);

    // Remap map n and k like it's described in [4].
    highp float n = gulbrandsenMapping_getn(rr, g);
    return vec2(n, gulbrandsenMapping_getk2(rr, n));
}

/**
 * @brief Fresnel reflectance term on dielectric-Conductor interface.
 * Copied from seblagarde.wordpress.com/2013/04/29/memo-on-fresnel-equations/
 *
 * @param iEta n component of eta
 * @param iEtak2 k2 component of eta. Dielectric Fresnel’s formulas are always
 * derive from conductor Fresnel’s formulas by settings k = 0.
 * @param iCosTheta
 *
 * @return
 */
highp vec3 fresnelDieletricConductor(
    highp vec3 iEta,
    highp vec3 iEtak2,
    highp float iCosTheta)
{
    highp float CosTheta2 = iCosTheta * iCosTheta;
    highp float SinTheta2 = 1.0 - CosTheta2;
    highp vec3 Eta2 = iEta * iEta;
    highp vec3 Etak2 = iEtak2;

    highp vec3 t0 = Eta2 - Etak2 - SinTheta2;
    highp vec3 a2plusb2 = sqrt(t0 * t0 + 4.0 * Eta2 * Etak2);
    highp vec3 t1 = a2plusb2 + CosTheta2;
    highp vec3 a = sqrt(0.5 * (a2plusb2 + t0));
    highp vec3 t2 = 2.0 * a * iCosTheta;
    highp vec3 Rs = (t1 - t2) / (t1 + t2);

    highp vec3 t3 = CosTheta2 * a2plusb2 + SinTheta2 * SinTheta2;
    highp vec3 t4 = t2 * SinTheta2;
    highp vec3 Rp = Rs * (t3 - t4) / (t3 + t4);

    // Prevent from negative.
    return 0.5 * max(Rp + Rs, 0.0);
}

/**
 * @brief The microfacet distribution. Equation 33 of [3]
 *
 * @param iRoughness
 * @param iCosTheta
 *
 * @return The distribution function D of GGX model.
 */
highp float ggxD(highp float iRoughness, highp float iCosTheta)
{
    highp float roughness2 = iRoughness * iRoughness;
    highp float cos2Theta = iCosTheta * iCosTheta;
    highp float sin2Theta = 1.0 - cos2Theta;
    highp float cos4Theta = cos2Theta * cos2Theta;
    highp float tan2Theta = sin2Theta / cos2Theta;
    highp float t = roughness2 + tan2Theta;
    highp float t2 = t * t;

    return roughness2 / (M_PI * cos4Theta * t2);
}

/**
 * @brief The Smith G1 equation for GGX. Equation 34 of [3].
 *
 * @param iRoughness
 * @param iCosTheta
 *
 * @return The Smith G1 term.
 */
highp float ggxG1(highp float iRoughness, highp float iCosTheta)
{
    highp float roughness2 = iRoughness * iRoughness;
    highp float cos2Theta = iCosTheta * iCosTheta;
    highp float sin2Theta = 1.0 - cos2Theta;
    highp float tan2Theta = sin2Theta / cos2Theta;

    highp float t = 1.0 + roughness2 * tan2Theta;
    return 2.0 / (1.0 + sqrt(t));
}

/**
 * @brief The bidirectional shadowing-masking factor. Equation 23 of [3].
 *
 * @param iRoughness
 * @param iNdotL i.m
 * @param iNdotV o.m
 *
 * @return The shadowing-masking term for GGX.
 */
highp float ggxG(highp float iRoughness, highp float iNdotL, highp float iNdotV)
{
    return ggxG1(iRoughness, iNdotL) * ggxG1(iRoughness, iNdotV);
}

/**
 * @brief The reflection term (BRDF) of the microsurface BSDFs with GGX
 * distribution.
 *
 * TODO: there is no m (Microsurface normal)
 *
 * @param N
 * @param V
 * @param L
 * @param roughness
 * @param fresnelN
 * @param fresnelK
 */
highp vec3 ggxBRDF(
    highp vec3 N,
    highp vec3 V,
    highp vec3 L,
    highp float roughness,
    highp vec3 fresnelN,
    highp vec3 fresnelK)
{
    highp vec3 H = normalize(L + V);
    highp float VdotH = dot(V, H);
    highp float LdotH = VdotH;
    highp float NdotH = dot(N, H);
    highp float NdotV = dot(N, V);
    highp float NdotL = dot(N, L);

    if (NdotH <= 0.0)
    {
        // Backface
        return vec3(0.0, 0.0, 0.0);
    }

    // GGX distribution.
    highp float D = ggxD(roughness, NdotH);

    // Geometric attenuation factor.
    highp float G = ggxG(roughness, NdotL, NdotV);

    //  The Fresnel term.
    highp vec3 F = fresnelDieletricConductor(fresnelN, fresnelK, LdotH);

    // Equation 20 of [3]
    return F * G * D / (4.0f * NdotV * NdotL);
}

/**
 * @brief Converts direction to the latitude/longitude.
 *
 * @param dir The xyz vector
 *
 * @return The latitude/longitude direction.
 */
highp vec2 envToUV(highp vec3 dir)
{
    return vec2(
        (atan(dir.x / dir.z) + M_PI) / (2.0 * M_PI),
        (asin(-dir.y) + M_PI / 2.0) / M_PI);
}

/**
 * @brief Compute the specular component of the surface.
 *
 * @return The specular component.
 */
highp vec3 reflectionComponent(
    highp vec3 iL,
    highp vec3 iN,
    highp vec3 iV,
    highp vec3 iReflectance,
    highp vec3 iEdge,
    highp float iRoughness)
{
    // Convert user-friendly reflectance and edge color to n and k of the
    // complex index of refraction.
    highp vec2 etaR = gulbrandsenMapping(iReflectance.r, iEdge.r);
    highp vec2 etaG = gulbrandsenMapping(iReflectance.g, iEdge.g);
    highp vec2 etaB = gulbrandsenMapping(iReflectance.b, iEdge.b);
    highp vec3 etan = vec3(etaR.x, etaG.x, etaB.x);
    highp vec3 etak = vec3(etaR.y, etaR.y, etaR.y);

    highp vec3 R = normalize(reflect(-iV, iN));

    highp float theta = max(dot(iN, iV), 0.0);

    // Environment.
    highp vec3 reflection = texture(gSamplerEnv, envToUV(R)).xyz *
        fresnelDieletricConductor(etan, etak, theta);

    // GGX specular.
    highp vec3 specular = ggxBRDF(iN, iV, iL, iRoughness, etan, etak);

    return reflection + specular;
}

/**
 * @brief Ray-marching.
 *
 * @param iP The ray origin.
 * @param iDir The ray direction.
 * @param maxlength The maximum tracing distance.
 *
 * @return The UV coordinate of the intersection of the ray and the scene.
 */
highp vec2 trace(highp vec3 iP, highp vec3 iDir, highp float maxlength)
{
    highp float ray = 0.0;
    highp float delta = 1.25;
    highp float distanceCurrent;
    highp float distanceExpected;
    highp vec2 Pscreen;
    do
    {
        ray += delta;
        if (ray > maxlength)
        {
            return vec2(0.0, 0.0);
        }

        highp vec3 origin = iP + iDir * ray;
        delta *= 1.025;

        highp vec4 Pndc = gMVP * vec4(origin, 1);
        Pndc /= Pndc.w;

        Pscreen = Pndc.xy / 2. + 0.5;
        if (Pscreen.x > 1. || Pscreen.x < 0. || Pscreen.y > 1. ||
            Pscreen.y < 0.)
        {
            return vec2(0.0, 0.0);
        }

        highp vec3 P = texture(gSamplerP, Pscreen).xyz;

        distanceExpected = length(P.xyz - gE);
        distanceCurrent = length(origin.xyz - gE);
    } while (distanceExpected > distanceCurrent);

    return Pscreen;
}

void main()
{
    highp vec2 frame = vec2(uv.x, 1.0 - uv.y);
    highp vec4 color = texture(gSamplerC, frame);
    highp vec3 reflectance = color.xyz;
    highp vec3 edge = vec3(1.0, 1.0, 1.0) - vec3(1.0, 0.25, 0.0) * 0.01;
    highp float roughness = 0.12;

    highp vec3 P = texture(gSamplerP, frame).xyz;

    if (color.w == 0.0)
    {
        // We are here because there is no geo. Return environment.
        highp vec3 In = normalize(P - gE);
        highp vec3 environment = texture(gSamplerEnv, envToUV(In)).xyz;
        Ci = vec4(environment, 1.0);
    }
    else
    {
        highp vec3 Nn = texture(gSamplerN, frame).xyz;
        highp vec3 Vn = normalize(gE - P);
        highp vec3 Ln = normalize(gLightPos - P);
        highp vec3 R = normalize(reflect(-Vn, Nn));

        highp vec3 diffuse = color.xyz * 0.25 * max(dot(Nn, Ln), 0.0);

        // Reflection. First, try to trace.
        highp vec2 traced = trace(P, R, 50.0);

        highp vec3 reflection;
        highp float tracedWeight = 1.0;
        if (traced.x > 0.0 && traced.y > 0.0)
        {
            highp vec4 tracedC = texture(gSamplerC, traced);
            if (tracedC.w > 0.0)
            {
                highp vec3 tracedReflectance = tracedC.xyz;

                // Convert user-friendly reflectance and edge color to n and k
                // of the complex index of refraction.
                // TODO: We compute the same indexes twice.
                highp vec2 etaR = gulbrandsenMapping(reflectance.r, edge.r);
                highp vec2 etaG = gulbrandsenMapping(reflectance.g, edge.g);
                highp vec2 etaB = gulbrandsenMapping(reflectance.b, edge.b);
                highp vec3 etan = vec3(etaR.x, etaG.x, etaB.x);
                highp vec3 etak = vec3(etaR.y, etaR.y, etaR.y);

                highp vec3 tracedP = texture(gSamplerP, traced).xyz;
                highp float traceLenght = length(P - tracedP);
                highp vec3 tracedNn = texture(gSamplerN, traced).xyz;
                highp vec3 tracedLn = normalize(gLightPos - tracedP);

                highp float theta = max(dot(Nn, Vn), 0.0);

                // TODO: We compute the same fresnel twice.
                reflection = reflectionComponent(
                                 tracedLn,
                                 tracedNn,
                                 -R,
                                 tracedReflectance,
                                 edge,
                                 roughness) *
                    fresnelDieletricConductor(etan, etak, theta);
                tracedWeight = traceLenght * 0.02;
                tracedWeight *= tracedWeight;
            }
        }

        if (tracedWeight > 0.0)
        {
            // Get the environment and specular and mix it with the reflection.
            reflection = reflection * (1.0 - tracedWeight) +
                reflectionComponent(Ln, Nn, Vn, reflectance, edge, roughness) *
                    tracedWeight;
        }

        Ci = vec4(diffuse + reflection, 1.0);
    }
}
