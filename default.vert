layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 displayfragC;
layout(location = 3) in float angle;
uniform highp mat4 mvp;
uniform highp int nrows;
uniform highp int id;
out vec3 fragP;
out vec3 fragN;
out vec3 fragC;
out vec3 fragID;

#define M_PI 3.141592653589793

mat4 rotationMatrix(float a)
{
    float s = sin(M_PI * a / 2.0);
    float c = cos(M_PI * a / 2.0);
    return mat4(c, 0., s, 0., 0., 1., 0., 0., -s, 0., c, 0., 0., 0., 0., 1.);
}

mat4 translationMatrix(vec2 offset)
{
    float x = offset.x;
    float y = offset.y;
    return mat4(1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., x, 0., y, 1.);
}

void main()
{
    // Compute object offset depending on the instance ID.
    vec2 offset;
    if (nrows > 1)
    {
        ivec2 index = ivec2(gl_InstanceID % nrows, gl_InstanceID / nrows);
        float step = 3.0 / float(nrows - 1);
        offset = vec2(
            (-1.5 + float(index.x) * step) * 105.0,
            (-1.5 + float(index.y) * step) * 105.0);
    }
    else
    {
        offset = vec2(0.0, 0.0);
    }

    mat4 local = translationMatrix(offset) * rotationMatrix(angle);

    fragC = displayfragC;
    fragP = vec3(local * vec4(vertex, 1.0f));
    fragN = mat3(local) * normal;
    float floatID = float(gl_InstanceID);
    fragID = vec3(
        float(id) * 0.1, mod(floatID, 10.0) * 0.1, floor(floatID / 10.0) * 0.1);

    gl_Position = mvp * local * vec4(vertex, 1.0f);
}
