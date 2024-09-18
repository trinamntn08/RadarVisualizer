#version 330 core
in float intensity;
out vec4 FragColor;

void main()
{
	float alpha = (intensity > 0.0) ? 1.0 : 0.0;
	FragColor = vec4(0.0, intensity, 0.0, alpha); // Green color with alpha channel based on intensity
}