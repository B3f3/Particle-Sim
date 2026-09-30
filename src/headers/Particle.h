#pragma once
#include "Vec2.h"

struct Particle {
    Vec2   position;
    Vec2   velocity;
    Vec2   acceleration;
    double mass   = 1.0;
    double radius = 1.0;
};