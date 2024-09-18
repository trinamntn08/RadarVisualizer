#include "radarRenderer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdlib> 
#include <stdexcept>

RadarRenderer::RadarRenderer(int width, int height)
				: width(width), height(height), m_windowGL(nullptr), shaderProgramPoints(0),
				shaderProgramQuads(0), VAO_points(0), VBO_points(0), VAO_quads(0), VBO_quads(0),
				FBO(0), texture(0), PBOs{ 0, 0 }, m_initialized(false)
{
}

RadarRenderer::~RadarRenderer()
{
	if (m_windowGL)
	{
		glfwMakeContextCurrent(m_windowGL); // Make the context current before cleanup
		glDeleteVertexArrays(1, &VAO_points);
		glDeleteBuffers(1, &VBO_points);
		glDeleteVertexArrays(1, &VAO_quads);
		glDeleteBuffers(1, &VBO_quads);
		glDeleteProgram(shaderProgramPoints);
		glDeleteProgram(shaderProgramQuads);
		glDeleteFramebuffers(1, &FBO);
		glDeleteTextures(1, &texture);
		for (int i = 0; i < 2; ++i)
		{
			if (persistentPtr[i])
			{
				glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
				persistentPtr[i] = nullptr;
			}
			glDeleteBuffers(1, &PBOs[i]);
		}
		glfwDestroyWindow(m_windowGL);
		glfwTerminate();
	}
}

void RadarRenderer::init()
{
	if (!m_initialized)
	{
		// Initialize GLFW
		if (!glfwInit())
		{
			throw std::runtime_error("Failed to initialize GLFW");
		}
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		m_windowGL = glfwCreateWindow(width, height, "Radar Visualizer", nullptr, nullptr);
		if (!m_windowGL)
		{
			glfwTerminate();
			throw std::runtime_error("Failed to create GLFW window");
		}

		glfwMakeContextCurrent(m_windowGL);
		// Check if the context is current
		if (glfwGetCurrentContext() != m_windowGL)
		{
			std::cerr << "Error: OpenGL context is not current!" << std::endl;
			return;
		}
		// Initialize GLAD
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			throw std::runtime_error("Failed to initialize GLAD");
		}

		// Enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		createShaderProgramPoints();
		setupVertexDataPoints();

		createShaderProgramQuads();
		setupVertexDataQuad(nullptr, 0);

		initFBO();
		initPBO();

		m_initialized = true;
	}
}

void RadarRenderer::run()
{
	const float angleStep = static_cast<float>(M_PI) / 180.0f;  // 1-degree step
	const uint32_t nbrCells = MAX_CELLS;
	const uint32_t nbrLines = MAX_LINES;
	render(angleStep, nbrCells, nbrLines);
}
void RadarRenderer::render(float angleStep, uint32_t nbrCells, uint32_t nbrLines)
{
	const float EPSILON = 1e-4f;
	for (uint32_t i = 0; i < nbrLines; ++i)
	{
		RadarMessage radarMsg = RadarMessage::createRandom(i, angleStep, nbrCells);

		if (fabs(radarMsg.startAzimuth - radarMsg.endAzimuth) < EPSILON)
		{
			float currentAngle;
			std::vector<BYTE> lineData;

			prepareRadarData(radarMsg, currentAngle, lineData);

			loadDataPoints(currentAngle, nbrCells, lineData.data());
			renderPoints(currentAngle, nbrCells);
		}
		else
		{
			static std::vector<float> radarBuffer(MAX_LINES * MAX_CELLS);

			std::vector<float> vertices;
			generateDataQuads(radarMsg.startAzimuth, radarMsg.endAzimuth, radarMsg.intensity.size(),
								1.f, radarMsg.intensity.data(), vertices);

			radarBuffer.insert(radarBuffer.end(), vertices.begin(), vertices.end());
			renderQuad2(radarBuffer);
		}
	}
}

void RadarRenderer::prepareRadarData(const RadarMessage& radarMessage, float& currentAngle, std::vector<BYTE>& lineData)
{
	currentAngle = radarMessage.getAngle();
	lineData.resize(radarMessage.intensity.size());
	for (size_t j = 0; j < radarMessage.intensity.size(); ++j)
	{
		lineData[j] = static_cast<BYTE>(radarMessage.intensity[j] * 255.0f);
	}
}
void RadarRenderer::initFBO()
{
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MAX_CELLS, MAX_LINES, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw std::runtime_error("Framebuffer is not complete!");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RadarRenderer::initPBO()
{
	glGenBuffers(2, PBOs);
	for (int i = 0; i < 2; ++i)
	{
		glBindBuffer(GL_PIXEL_PACK_BUFFER, PBOs[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, MAX_CELLS * MAX_LINES * 4, nullptr, GL_STREAM_READ);  // Adjusted size and usage

		persistentPtr[i] = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
		if (!persistentPtr[i])
		{
			std::cerr << "Error: Unable to map PBO buffer (Index: " << i << ")." << std::endl;
		}
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	}
}


std::string RadarRenderer::loadShaderSource(const char* filePath)
{
	std::ifstream file(filePath);
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

GLuint RadarRenderer::compileShader(GLenum type, const char* source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		throw std::runtime_error("Error compiling shader: " + std::string(infoLog));
	}

	return shader;
}

void RadarRenderer::createShaderProgramPoints()
{
	char* shaderDirCStr = nullptr;
	size_t len = 0;

	errno_t err = _dupenv_s(&shaderDirCStr, &len, "SHADER_DIR");
	if (err || shaderDirCStr == nullptr || len == 0)
	{
		throw std::runtime_error("SHADER_DIR environment variable not set or empty.");
	}

	std::string shaderDir(shaderDirCStr);
	free(shaderDirCStr); 

	std::string vertexCode = loadShaderSource((shaderDir + "/PointVertexShader.glsl").c_str());
	std::string fragmentCode = loadShaderSource((shaderDir + "/PointFragmentShader.glsl").c_str());


	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

	shaderProgramPoints = glCreateProgram();
	glAttachShader(shaderProgramPoints, vertexShader);
	glAttachShader(shaderProgramPoints, fragmentShader);
	glLinkProgram(shaderProgramPoints);

	int success;
	glGetProgramiv(shaderProgramPoints, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgramPoints, 512, nullptr, infoLog);
		throw std::runtime_error("Error linking shader program: " + std::string(infoLog));
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void RadarRenderer::createShaderProgramQuads()
{
	char* shaderDirCStr = nullptr;
	size_t len = 0;

	errno_t err = _dupenv_s(&shaderDirCStr, &len, "SHADER_DIR");
	if (err || shaderDirCStr == nullptr || len == 0)
	{
		throw std::runtime_error("SHADER_DIR environment variable not set or empty.");
	}

	std::string shaderDir(shaderDirCStr);
	free(shaderDirCStr);

	std::string vertexCode = loadShaderSource((shaderDir + "/QuadVertexShader.glsl").c_str());
	std::string fragmentCode = loadShaderSource((shaderDir + "/QuadFragmentShader.glsl").c_str());

	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode.c_str());
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode.c_str());

	shaderProgramQuads = glCreateProgram();
	glAttachShader(shaderProgramQuads, vertexShader);
	glAttachShader(shaderProgramQuads, fragmentShader);
	glLinkProgram(shaderProgramQuads);

	int success;
	glGetProgramiv(shaderProgramQuads, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgramQuads, 512, nullptr, infoLog);
		throw std::runtime_error("Error linking shader program: " + std::string(infoLog));
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void RadarRenderer::setupVertexDataPoints()
{
	glGenVertexArrays(1, &VAO_points);
	glBindVertexArray(VAO_points);

	glGenBuffers(1, &VBO_points);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_points);

	glBufferData(GL_ARRAY_BUFFER, MAX_LINES * MAX_CELLS * sizeof(FLOAT), nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(FLOAT), (void*)0);
	glEnableVertexAttribArray(0);
}

void RadarRenderer::loadDataPoints(float& angle, const uint32_t& nbrCells, BYTE* lineData)
{
	// Ensure angle is within [0, 2PI]
	angle = fmod(angle, 2.0f * static_cast<float>(M_PI));

	int currentLine = static_cast<int>((angle / (2.0f * static_cast<float>(M_PI))) * MAX_LINES) % MAX_LINES;

	// Debugging output
	std::cout << "Angle: " << angle << ", Current Line: " << currentLine << ", Number of Cells: " << nbrCells << std::endl;

	// Validate that currentLine and nbrCells are within valid ranges
	if (currentLine < 0 || currentLine >= MAX_LINES || nbrCells > MAX_CELLS)
	{
		std::cerr << "Error: currentLine or nbrCells out of bounds." << std::endl;
		return;
	}

	std::vector<float> buffer(nbrCells, 0.0f);
	for (uint32_t i = 0; i < nbrCells; i++)
	{
		buffer[i] = static_cast<float>(lineData[i]) / 255.0f;
	}

	GLintptr offset = currentLine * MAX_CELLS * sizeof(FLOAT);
	GLsizeiptr size = nbrCells * sizeof(FLOAT);

	if (offset + size <= MAX_LINES * MAX_CELLS * sizeof(FLOAT))
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBO_points);
		glBufferSubData(GL_ARRAY_BUFFER, offset, size, buffer.data());
		checkGLErrors("updateVertexDataPoints");
	}
	else
	{
		std::cerr << "Error: glBufferSubData parameters out of bounds." << std::endl;
	}
}

void RadarRenderer::renderPoints(float& angle, uint32_t& nbrCells)
{
	glfwMakeContextCurrent(m_windowGL);
	//glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glBindVertexArray(VAO_points);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramPoints);
	glUniform1i(glGetUniformLocation(shaderProgramPoints, "nbrCells"), nbrCells);
	glUniform1f(glGetUniformLocation(shaderProgramPoints, "angle"), angle);

	int currentLine = static_cast<int>((angle / (2.0f * static_cast<float>(M_PI))) * MAX_LINES);
	int startIndex = currentLine * MAX_CELLS;
	glDrawArrays(GL_POINTS, startIndex, nbrCells);

	//glDrawArrays(GL_POINTS, 0, MAX_LINES * MAX_CELLS);

	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	checkGLErrors("render");

	glfwSwapBuffers(m_windowGL);
}

void RadarRenderer::renderQuad(float startAngle, float endAngle, int numCells, float radius)
{
	glUseProgram(shaderProgramQuads);
	glUniform1f(glGetUniformLocation(shaderProgramQuads, "startAngle"), startAngle);
	glUniform1f(glGetUniformLocation(shaderProgramQuads, "endAngle"), endAngle);
	glUniform1i(glGetUniformLocation(shaderProgramQuads, "numCells"), numCells);
	glUniform1f(glGetUniformLocation(shaderProgramQuads, "radius"), radius);

	glBindVertexArray(VAO_quads);
	glDrawArrays(GL_TRIANGLE_FAN, 0, numCells);
	glBindVertexArray(0);
	glfwSwapBuffers(m_windowGL);
}

void RadarRenderer::renderQuad2(const std::vector<float>& vertices)
{
	glBindBuffer(GL_ARRAY_BUFFER, VBO_quads);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shaderProgramQuads);
	glBindVertexArray(VAO_quads);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);

	glfwSwapBuffers(m_windowGL);
}

void RadarRenderer::setupVertexDataQuad(const uint8_t* rawData, int numCells)
{
	std::vector<float> intensities(numCells + 1); // Only intensities, vertexID calculated in shader
	intensities[0] = 0.0f; // Center intensity

	for (int i = 1; i <= numCells; ++i)
	{
		intensities[i] = static_cast<float>(rawData[i - 1]); // Intensity
	}

	glGenVertexArrays(1, &VAO_quads);
	glBindVertexArray(VAO_quads);

	glGenBuffers(1, &VBO_quads);
	glBindBuffer(GL_ARRAY_BUFFER, VBO_quads);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
}


void RadarRenderer::generateDataQuads(float startAngle, float endAngle, int numCells, float radius, 
										const float* intensities, std::vector<float>& vertices)
{
	// Calculate the step size for each cell based on the total radius and number of cells
	float stepSize = radius / numCells;
	std::vector<float> distance(numCells + 1);
	for (int i = 0; i <= numCells; ++i)
	{
		distance[i] = i * stepSize;
	}

	for (size_t i = 0; i < distance.size() - 1; ++i)
	{
		float radius1 = distance[i];
		float radius2 = distance[i + 1];

		float intensity = intensities[i];

		// First Triangle
		vertices.insert(vertices.end(), { radius1 * std::cos(startAngle), radius1 * std::sin(startAngle), 0.0f, intensity });
		vertices.insert(vertices.end(), { radius2 * std::cos(startAngle), radius2 * std::sin(startAngle), 0.0f, intensity });
		vertices.insert(vertices.end(), { radius1 * std::cos(endAngle), radius1 * std::sin(endAngle), 0.0f, intensity });

		// Second Triangle
		vertices.insert(vertices.end(), { radius1 * std::cos(endAngle), radius1 * std::sin(endAngle), 0.0f, intensity });
		vertices.insert(vertices.end(), { radius2 * std::cos(startAngle), radius2 * std::sin(startAngle), 0.0f, intensity });
		vertices.insert(vertices.end(), { radius2 * std::cos(endAngle), radius2 * std::sin(endAngle), 0.0f, intensity });
	}
}

void RadarRenderer::readBufferData(BYTE* cpuData, int width_, int height_)
{
	static int index = 0;
	int nextIndex = (index + 1) % 2;

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glReadBuffer(GL_COLOR_ATTACHMENT0);

	// Initiate the readback to the current PBO
	glBindBuffer(GL_PIXEL_PACK_BUFFER, PBOs[index]);
	glReadPixels(0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
	checkGLErrors("glReadPixels");

	// Use persistent mapped memory directly
	if (persistentPtr[nextIndex] != nullptr)
	{
		memcpy(cpuData, persistentPtr[nextIndex], width_ * height_);
	}
	else
	{
		std::cerr << "Error: Persistent pointer is null." << std::endl;
	}

	// Unbind the PBO
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Increment the index for double buffering
	index = nextIndex;

	checkGLErrors("readBufferData");
}


void RadarRenderer::checkGLErrors(const std::string& location)
{
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR)
	{
		std::string errorMessage;

		switch (error)
		{
		case GL_INVALID_ENUM:
			errorMessage = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			errorMessage = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			errorMessage = "GL_INVALID_OPERATION";
			break;
		case GL_OUT_OF_MEMORY:
			errorMessage = "GL_OUT_OF_MEMORY";
			break;
		default:
			errorMessage = "Unknown error";
			break;
		}

		std::cerr << "OpenGL error (" << location << "): " << errorMessage << " (" << error << ")" << std::endl;
	}
}
