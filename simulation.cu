#include <cuda_runtime.h>
#include "particle.h"
#include "Vector3.h"

// CUDA Kernel
__global__ void computeGravity(Particle* particles, int numParticles, double G, double dt) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < numParticles) {
        Particle& p1 = particles[i];
        Vector3 force(0, 0, 0);

        for (int j = 0; j < numParticles; ++j) {
            if (i == j) continue;
            Particle& p2 = particles[j];
            Vector3 displacement = p2.pos - p1.pos;
            double distance = displacement.magnitude();
            if (distance < 1e-2) continue; // Avoid singularity
            Vector3 direction = displacement / distance;
            double forceMagnitude = (G * p1.mass * p2.mass) / (distance * distance);
            force = force + direction * forceMagnitude;
        }

        // Update velocity and position
        p1.velocity = p1.velocity + force * (dt / p1.mass);
        p1.pos = p1.pos + p1.velocity * dt;
    }
}

extern "C" void updateParticlesCUDA(Particle * particles, int numParticles, double dt, double G) {
    // Allocate memory on the GPU
    Particle* d_particles;
    cudaMalloc(&d_particles, numParticles * sizeof(Particle));

    // Copy data from host (CPU) to device (GPU)
    cudaMemcpy(d_particles, particles, numParticles * sizeof(Particle), cudaMemcpyHostToDevice);

    // Launch the CUDA kernel with an appropriate number of threads and blocks
    int blockSize = 256;
    int numBlocks = (numParticles + blockSize - 1) / blockSize;
    computeGravity << <numBlocks, blockSize >> > (d_particles, numParticles, G, dt);

    // Copy the results back from the GPU to the CPU
    cudaMemcpy(particles, d_particles, numParticles * sizeof(Particle), cudaMemcpyDeviceToHost);

    // Free the memory on the GPU
    cudaFree(d_particles);
}