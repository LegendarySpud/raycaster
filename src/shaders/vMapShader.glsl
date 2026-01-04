#version 460 core
layout (location = 0) in vec3 aPos;
uniform float scale;
uniform vec2 pos;

void main() {
	vec3 newPos;
	newPos.x = ((aPos.x+1.0f)/2.0f) * scale + pos.x*scale - 1;
	newPos.y = ((aPos.y+1.0f)/2.0f) * scale + pos.y*scale;
	newPos.z = aPos.z;
	gl_Position = vec4(newPos, 1.0f);
}