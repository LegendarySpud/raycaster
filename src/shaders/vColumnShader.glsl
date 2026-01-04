#version 460 core
layout (location = 0) in vec2 vPos;

out vec3 vColour;

void main() {

    gl_Position = vec4(vPos.x, vPos.y, 0.0, 1.0);
    vColour = vec3(0.0f, 0.0f, 1.0f);
}