#version 460 core
out vec4 FragColour;
in vec3 vColour;

void main() {
	FragColour = vec4(vColour, 1.0f);
}