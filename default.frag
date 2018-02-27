// Copyright 2018 Victor Yudin. All rights reserved.

in highp vec3 fragP;
in highp vec3 fragN;
in highp vec3 fragC;
in highp vec3 fragID;

layout(location = 1) out highp vec4 aovP;
layout(location = 2) out highp vec4 aovN;
layout(location = 3) out highp vec4 aovID;
layout(location = 4) out highp vec4 aovC;

void main()
{
    highp vec3 Nn = normalize(fragN);

    aovP = vec4(fragP, 1.0);
    aovN = vec4(Nn, 0.0);
    aovC = vec4(fragC, 1.0);
    aovID = vec4(fragID, 1.0f);
}
