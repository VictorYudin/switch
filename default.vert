layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 displayColor;
layout(location = 3) in float angle;
uniform highp mat4 mvp;
uniform highp vec3 camera;
uniform highp int nrows;
uniform highp int id;
out vec3 vert;
out vec3 vertNormal;
out vec3 color;
out vec3 vertObjectID;
out vec4 V;

mat4 rotationMatrix(float a)
{
    float s = sin(3.1415926 * a / 2.0);
    float c = cos(3.1415926 * a / 2.0);
    return mat4(
            c, 0.0, s, 0.0,
            0.0, 1.0, 0.0, 0.0,
            -s, 0.0, c, 0.0,
            0.0, 0.0, 0.0, 1.0);
}

void main()
{
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

    mat4 verticalFlip = mat4(
            1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0f, 0.0, 1.0);
    mat4 world = mat4(
            1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            offset.x, 0.0f, offset.y, 1.0) * rotationMatrix(angle);

    color = displayColor;
    vert = vec3(world * vec4(vertex, 1.0f));
    vertNormal = mat3(world) * normal;
    float floatID = float(gl_InstanceID);
    vertObjectID = vec3(
            float(id) * 0.1,
            mod(floatID, 10.0) * 0.1,
            floor(floatID / 10.0) * 0.1);
    gl_Position = verticalFlip * mvp * world * vec4(vertex, 1.0f);
    V = vec4(camera, 1.0f) - world * vec4(vertex, 1.0f);
}
