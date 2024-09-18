cmake_minimum_required(VERSION 3.10)  # Set the minimum CMake version
project(RadarVisualizer)  # Project name

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify the directories for the dependencies
set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/source")

# Set the environment variable for shaders
set(SHADER_DIR "${SOURCE_DIR}/shaders") 
message(STATUS "SHADER_DIR: ${SHADER_DIR}")  # Print the value for confirmation

# Include the directories for the dependencies
include_directories(
    ${DEPS_DIR}/glad
    ${DEPS_DIR}/glfw
    ${DEPS_DIR}/glm
)

# Find OpenGL
find_package(OpenGL REQUIRED)  # Ensure OpenGL is found

# Create a list of glad source files
file(GLOB GLAD_SOURCES "${SOURCE_DIR}/glad.c")

# Create a list of source files
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.h")

# Add the executable target
add_executable(RadarVisualizer ${SOURCES} ${GLAD_SOURCES})

# Specify the path to the GLFW library
set(GLFW_LIB_DIR "${DEPS_DIR}/glfw")

# Set environment variable for debugging in Visual Studio
set_target_properties(RadarVisualizer PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "SHADER_DIR=${SHADER_DIR}"
)

# Add the GLFW library directory to the target
target_link_directories(RadarVisualizer PRIVATE ${GLFW_LIB_DIR})

# Link the libraries
target_link_libraries(RadarVisualizer
    glfw3
	OpenGL::GL
)

# Set the output directory for the built executable
set_target_properties(RadarVisualizer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin" 
)
