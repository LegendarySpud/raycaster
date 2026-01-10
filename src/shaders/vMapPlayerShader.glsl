#version 460 core
layout (location = 0) in vec3 aPos;
uniform float scale;
uniform vec2 pos;

void main() {
	vec3 newPos;
	newPos.x = 0-(aPos.x*0.3f*scale + pos.x*scale);
	newPos.y = aPos.y*0.3f*scale + pos.y*scale;
	newPos.z = aPos.z;
	gl_Position = vec4(newPos, 1.0f);
}