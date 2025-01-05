#include <GL/glew.h>
#include <GLFW/glfw3.h>


#define _USE_MATH_DEFINES

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <omp.h>
#include "Vector3.h"
#include "particle.h"
#include "OctTree.h"

const double G = 6.67430e-10;
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 instanceOffset;

    uniform mat4 MVP; // Model-View-Projection matrix

    void main() {
        gl_Position = MVP * vec4(aPos + instanceOffset, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White color
    }
)";

void updateParticles(std::vector<Particle>& particles, double dt) {
#pragma omp parallel for
    for (size_t i = 0; i < particles.size(); ++i) {
        Particle& p = particles[i];
        p.pos.x += p.velocity.x * dt;
        p.pos.y += p.velocity.y * dt;
        p.pos.z += p.velocity.z * dt;
    }
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
    }
    return shader;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1400, 1000, "Gravity Simulation", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewInit();

    // Compile shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float particleVertices[] = { 0.0f, 0.0f, 0.0f };

    GLuint VAO, VBO, instanceVBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(particleVertices), particleVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Instance positions
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);

    // Initialize particles
    std::vector<Particle> particles;
    srand(static_cast<unsigned>(time(nullptr)));

    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 25; j++) {
            double x = .5 * cos(i / 50.0 * 2 * M_PI) * sin(M_PI * j / 25.0);
            double y = .5 * sin(2 * M_PI * i / 50.0) * sin(M_PI * j / 25.0);
            double z = .5 * cos(M_PI * j / 25.0);
            particles.emplace_back(Vector3(x, y, z), 10000, Vector3(0, 0, 0));
        }
    }

    double dt = .1;
    int frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    glUseProgram(shaderProgram);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        // Update particles
        Tree oct = Tree(particles, 3, .1, G);
        oct.makeTree();
        oct.gravity();
        updateParticles(particles, dt);

        // Update instance buffer
        std::vector<float> positions;
        for (const Particle& p : particles) {
            positions.push_back(p.pos.x);
            positions.push_back(p.pos.y);
            positions.push_back(p.pos.z);
        }
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(float), positions.data(), GL_DYNAMIC_DRAW);

        // Calculate MVP matrix
        glm::mat4 model = glm::mat4(1.0f); // Identity matrix
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 1.0f, 1.0f)); // Fixed 45-degree rotation around (1,1,1)
        glm::mat4 view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 2.0f), // Camera position
            glm::vec3(0.0f, 0.0f, 0.0f), // Look at origin
            glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
        );
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), // Field of view
            1400.0f / 1000.0f,   // Aspect ratio
            0.1f,                // Near plane
            100.0f               // Far plane
        );
        glm::mat4 MVP = projection * view * model;

        // Pass MVP matrix to shader
        GLuint mvpLoc = glGetUniformLocation(shaderProgram, "MVP");
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(MVP));

        // Render particles
        glBindVertexArray(VAO);
        glDrawArraysInstanced(GL_POINTS, 0, 1, particles.size());
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        // FPS calculation
        frameCount++;
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = currentTime - startTime;

        if (elapsed.count() >= 1.0) {
            double fps = frameCount / elapsed.count();
            std::cout << "FPS: " << fps << std::endl;
            frameCount = 0;
            startTime = currentTime;
        }
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}