#pragma once
#include "Vector3.h"
struct Particle {
    Vector3 pos = Vector3(0, 0, 0);
    Vector3 velocity = Vector3(0, 0, 0);
    double mass = 0;
    Particle() {

    }
    Particle(const Vector3& pos, double mass, const Vector3& velocity)
        :pos(pos), mass(mass), velocity(velocity) {}
};


