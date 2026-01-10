#version 460 core
layout (location = 0) in vec3 vPos;

uniform mat4 view;
uniform mat4 projection;

out vec3 vColour;

void main() {

    gl_Position = projection * view * vec4(vPos, 1.0f);
    vColour = vec3(0.0f, 0.0f, 1.0f);
}