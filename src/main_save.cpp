#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include "headers/Vec2.h"
#include "headers/Particle.h"
#include "raylib.h"

// Brief's units: G = 1, masses 1-1000, distances 1-1000.
// SI's 6.674e-11 with masses of 2 gives accelerations around 1e-11 - nothing
// would appear to move at all.
const double G   = 6.67430e-11;
const double eps = 0.0;          // or 1.0 — negligible at these distances
const int WINDOW_H = 768;
const int WINDOW_W = 1280;

void computeFs(std::vector<Particle>& particles){
    for (Particle& p : particles) {
        p.acceleration = {0.0, 0.0};
    }

    for (size_t i = 0; i < particles.size(); i++) {
        for (size_t j = i + 1; j < particles.size(); j++) {
            Particle& a = particles[i];
            Particle& b = particles[j];

            Vec2   d  = b.position - a.position;   // points from a toward b
            double r2 = d.lengthSquared();
            Vec2   dir = d.normalise();

            // a = G*M/r^2 - the OTHER particle's mass, since dividing force by
            // this particle's own mass cancels it.
            a.acceleration += dir * (G * b.mass / (r2 + eps * eps));
            b.acceleration -= dir * (G * a.mass / (r2 + eps * eps));
        }
    }

}

void step(std::vector<Particle>& particles, double dt) {
    for (Particle& p : particles) {
        p.velocity += p.acceleration * (0.5 * dt);
        p.position += p.velocity * dt;
    }

    computeFs(particles);

    for (Particle& p : particles)
        p.velocity += p.acceleration * (0.5 * dt);
}

double totalEnergy(const std::vector<Particle>& particles) {
    double ke = 0.0;
    for (const Particle& p : particles) {
        ke += 0.5 * p.mass * p.velocity.lengthSquared();
    }

    double pe = 0.0;
    for (size_t i = 0; i < particles.size(); i++) {
        for (size_t j = i + 1; j < particles.size(); j++) {
            // Pairs again, i < j. Summing ordered pairs would double-count.
            double r2 = (particles[j].position - particles[i].position).lengthSquared();
            pe -= G * particles[i].mass * particles[j].mass / std::sqrt(r2 + eps * eps);
        }
    }

    return ke + pe;
}

Vec2 totalMomentum(const std::vector<Particle>& particles) {
    Vec2 p = {0.0, 0.0};
    for (const Particle& q : particles) {
        p += q.velocity * q.mass;
    }
    return p;
}

void render(const std::vector<Particle>& particles) {
    const double scale = 20.0;
    BeginDrawing();
    ClearBackground(BLACK);
    for (const Particle& p : particles) {
        float sx = (float)( p.position.x * scale + WINDOW_W / 2.0 );
        float sy = (float)( WINDOW_H / 2.0 - p.position.y * scale );
        DrawCircleV({sx, sy}, (float)p.radius, RED);
        std::cout<< p.acceleration.length()<< std::endl;
    }
    EndDrawing();
}

int main() {

    const double dt = 0.01;
    std::cout << "dt: "<< dt << std::endl;
    double accumulator = 0.0; 

    std::vector<Particle> particles;
    // Earth
    particles.push_back({{0, 0}, {0, -12.44}, {0,0}, 5.972e24, 60});
    // Moon
    particles.push_back({{3.844e8, 0}, {0, 1012.09}, {0,0}, 7.342e22, 17});

    computeFs(particles);   //so that first loop is correct

    InitWindow(WINDOW_W, WINDOW_H, "particle sim");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        accumulator += GetFrameTime();
        while (accumulator >= dt) {
            step(particles, dt);
            accumulator -= dt;
        }
        render(particles);

    }CloseWindow();

    return 0;
}