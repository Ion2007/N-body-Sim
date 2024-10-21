
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
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
    void main() {
        gl_Position = vec4(aPos, 1.0);
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
    for (Particle& p : particles) {
        p.pos.x += p.velocity.x * dt;
        p.pos.y += p.velocity.y * dt;
        p.pos.z += p.velocity.z * dt;
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(1400, 1000, "Gravity Simulation", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    std::vector<Particle> particles;
    srand(time(NULL));
  
    for (int i = 0; i < 50; i++) {
        for (int j = 0; j < 25; j++) {
            double x = .5 * cos(i / 50.0 * 2 * M_PI) * sin(M_PI * j / 25.0);
            double y = .5 * sin(2 * M_PI * i / 50.0) * sin(M_PI * j / 25.0);
            double z = .5 * cos(M_PI * j / 25.0);
            particles.emplace_back(Vector3(x, y, z), 10000, Vector3(0, 0, 0));
        }
    }
    
    /*
    particles.emplace_back(Vector3(0, 0, .5), 10000, Vector3(0, 0, 0));
    particles.emplace_back(Vector3(.5, 0, 0), 10000, Vector3(0, 0, 0));
    particles.emplace_back(Vector3(-0.25, .43, 0), 10000, Vector3(0, 0, 0));
    */

    double dt = .1;
    double x = 0.0;
    int frameCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();
        glRotatef(45.0f, 45.0f, 45.0f, 0.0f);
        x += .10f;

        Tree oct = Tree(particles,3, .2, G);
        oct.makeTree();
        oct.gravity();
        updateParticles(particles, dt);

        glPointSize(2.0f);
        glBegin(GL_POINTS);

        for (const Particle& p : particles) {
            glColor3f(1.0f, 1.0f, 1.0f); // White color
            glVertex3f(p.pos.x, p.pos.y, p.pos.z);
        }
        glEnd();

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

    glfwTerminate();
    return 0;
}
