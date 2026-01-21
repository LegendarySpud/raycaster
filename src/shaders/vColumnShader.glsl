#version 460 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in float texType;
layout (location = 2) in vec2 texPos;

uniform vec3 aColour;
out vec3 vColour;
out vec2 texCoord;

void main() {
	float projZ = 1.0f - (1.0f / (vPos.z+1));
    gl_Position = vec4(vPos.xy, projZ, 1.0f);
    vColour = aColour;

    texCoord.x = 0.25f*texPos.x + 0.25f*mod(texType, 4);
    texCoord.y = 0.25f*texPos.y;// + 0.25f*(texType/4);
}