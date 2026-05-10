#include <glad/gl.h>      // GLAD Version 2 Header
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

// Vertex Shader: Defines position and point scalar size
const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    gl_PointSize = 10.0;
}
)glsl";

// Fragment Shader: Evaluates flat color for the rasterized points
const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(0.2f, 0.8f, 0.5f, 1.0f); // RGBA
}
)glsl";

// Utility function to compile shaders
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

int main() {
    // 1. Initialize GLFW and Window Context
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL 2D Points (GLAD 2)", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // 2. Initialize GLAD 2 to load OpenGL function pointers
    // gladLoadGL returns the loaded version number instead of a boolean
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::cerr << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }
    
    // Enable modification of point size within the vertex shader
    glEnable(GL_PROGRAM_POINT_SIZE);

    // 3. Compile Shaders and Link Program
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 4. Define Vertex Data
    std::vector<float> points = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.0f,  0.5f,
        -0.8f,  0.2f,
         0.7f,  0.6f
    };

    // 5. Generate and Bind Buffers
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);

    // 6. Specify Memory Layout for the Shader
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // 7. Render Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        
        glDrawArrays(GL_POINTS, 0, points.size() / 2);

        glfwSwapBuffers(window);
    }

    // 8. Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}