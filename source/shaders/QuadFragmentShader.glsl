#version 330 core
in float intensity;
out vec4 color;
void main() {
	color = vec4(0.0, intensity, 0.0, 1.0); // Set intensity in green channel
}