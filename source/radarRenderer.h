#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glfw3.h>

#include"radarMessage.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int MAX_LINES = 1024;
const int MAX_CELLS = 1024;

class RadarRenderer
{
public:
	RadarRenderer(int width, int height);
	~RadarRenderer();

	void init();

	void run();

	// Processing Radar Data
	void render(float angleStep, uint32_t nbrCells, uint32_t nbrLines);
	void prepareRadarData(const RadarMessage& radarMessage, float& currentAngle, std::vector<BYTE>& lineData);

	// Render points
	void loadDataPoints(float& angle, const uint32_t& nbrCells, BYTE* lineData);
	void renderPoints(float& angle, uint32_t& nbrCells);

	//Render quads
	void setupVertexDataQuad(const uint8_t* rawData, int numCells);
	void generateDataQuads(float startAngle, float endAngle, int numCells, float radius, const float* intensities, std::vector<float>& vertices);

	//TODO
	void renderQuad(float startAngle, float endAngle, int numCells, float radius); 
	//Use it for now
	void renderQuad2(const std::vector<float>& vertices); 

	// Sendback data from GPU to CPU
	void readBufferData(BYTE* cpuData, int width_, int height_);

	void initFBO();
	void initPBO();

private:
	std::string loadShaderSource(const char* filePath);
	GLuint compileShader(GLenum type, const char* source);
	void createShaderProgramPoints();
	void createShaderProgramQuads();

	void setupVertexDataPoints();

	void checkGLErrors(const std::string& location);

private:
	int m_initialized = false;
	int width;
	int height;
	GLFWwindow* m_windowGL;  // Store the hidden GLFW window
	GLuint shaderProgramPoints;
	GLuint shaderProgramQuads;
	GLuint VAO_points, VBO_points;
	GLuint VAO_quads, VBO_quads;
	GLuint FBO;
	GLuint texture; // Texture attached to the FBO
	GLuint PBOs[2]; // Double-buffered PBOs for asynchronous data transfer
	GLubyte* persistentPtr[2] = { nullptr, nullptr };
};
