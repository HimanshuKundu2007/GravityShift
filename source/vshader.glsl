#version 150

in vec4 vPosition;
in vec3 vNormal;

out vec3 fPositionEye;
out vec3 fNormalEye;

uniform mat4 ModelView;
uniform mat4 Projection;

void main()
{
    // Transform vertex position into eye (camera) coordinates
    vec4 eyePos = ModelView * vPosition;
    fPositionEye = eyePos.xyz;

    // Transform surface normal vector into eye coordinates
    fNormalEye = (ModelView * vec4(vNormal, 0.0)).xyz;

    // Project into clip coordinates
    gl_Position = Projection * eyePos;
}
