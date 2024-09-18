#version 330 core
uniform float angle; // Specific angle for the current line
uniform int nbrCells; // Number of cells per line

layout(location = 0) in float aIntensity; // Intensity as unsigned integer

out float intensity;

#define M_PI 3.14159265358979323846
#define MAX_CELLS 4048 // or another appropriate value

void main()
{
	// Calculate the line index and distance based on gl_VertexID
	int lineIndex = gl_VertexID / MAX_CELLS; // Determines the specific line
	float distance = float(gl_VertexID % MAX_CELLS) / float(MAX_CELLS - 1); // Position along the line (normalized)

	// Calculate the specific angle for the line
	float lineAngle = angle; // The angle for this specific line is directly provided

	// Convert polar coordinates to Cartesian coordinates
	vec2 position = vec2(distance * cos(lineAngle), distance * sin(lineAngle));

	// Set the final position in clip space
	gl_Position = vec4(position, 0.0, 1.0);

	// Normalize and pass the intensity to the fragment shader
	intensity = aIntensity;
}