// Copyright 2018 Victor Yudin. All rights reserved.

layout(location = 0) in highp vec3 P;
layout(location = 1) in highp vec2 st;

out highp vec2 uv;

void main()
{
    uv = st;
    gl_Position = vec4(P, 1.0);
}
