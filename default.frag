uniform highp vec3 lightPos;
in highp vec3 vert;
in highp vec3 vertNormal;
in highp vec3 color;
in highp vec3 vertObjectID;
in highp vec4 V;
layout(location = 0) out highp vec4 fragColor;
layout(location = 1) out highp vec4 objectID;

highp float ggx(
    highp vec3 N,
    highp vec3 V,
    highp vec3 L,
    highp float roughness,
    highp float F0)
{
    highp float alpha = roughness * roughness;
    highp vec3 H = normalize(L + V);
    highp float dotLH = max(0.0, dot(L, H));
    highp float dotNH = max(0.0, dot(N, H));
    highp float dotNL = max(0.0, dot(N, L));
    highp float alphaSqr = alpha * alpha;
    highp float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;
    highp float D = alphaSqr / (3.141592653589793 * denom * denom);
    highp float F = F0 + (1.0 - F0) * pow(1.0 - dotLH, 5.0);
    highp float k = 0.5 * alpha;
    highp float k2 = k * k;
    return dotNL * D * F / (dotLH * dotLH * (1.0 - k2) + k2);
}

void main()
{
    highp vec3 L = lightPos - vert;
    highp vec3 nL = normalize(L);
    highp vec3 nN = normalize(vertNormal);
    highp float NL = max(dot(nN, nL), 0.0);
    highp float g = ggx(nN, V.xyz, nL, 0.5, 0.1);
    fragColor = vec4(color * NL + vec3(g, g, g), 1.0);
    objectID = vec4(vertObjectID, 1.0f);
}
