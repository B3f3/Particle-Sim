#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <deque>
#include <cmath>
#include "headers/Vec2.h"
#include "headers/Particle.h"
#include "raylib.h"

// SI units: metres, kilograms, seconds.
const double G   = 6.67430e-11;
const double eps = 0.0;

const int WINDOW_H = 768;
const int WINDOW_W = 1280;

// --- rendering constants -----------------------------------------------
//
// The Moon's orbit is 3.844e8 m across. Mapping that to ~300 pixels gives the
// scale. At that scale the Earth's 6371 km radius is 5 pixels and the Moon's
// 1737 km is ONE pixel - the real system is almost entirely empty space, so
// bodies get fixed pixel sizes and a caption saying so.
const double ORBIT_R   = 3.844e8;
const double SCALE     = 300.0 / ORBIT_R;

// A lunar month is 2.36e6 seconds. Watching in real time would take 27 days,
// so compress it: one orbit in roughly ten seconds of wall clock.
const double TIME_SCALE = 236000.0;

const size_t TRAIL_MAX = 500;

void computeFs(std::vector<Particle>& particles) {
    for (Particle& p : particles) {
        p.acceleration = {0.0, 0.0};
    }

    for (size_t i = 0; i < particles.size(); i++) {
        for (size_t j = i + 1; j < particles.size(); j++) {
            Particle& a = particles[i];
            Particle& b = particles[j];

            Vec2   d   = b.position - a.position;   // points from a toward b
            double r2  = d.lengthSquared();
            Vec2   dir = d.normalise();

            // a = G*M/r^2 - the OTHER particle's mass, since dividing force by
            // this particle's own mass cancels it.
            a.acceleration += dir * (G * b.mass / (r2 + eps * eps));
            b.acceleration -= dir * (G * a.mass / (r2 + eps * eps));
        }
    }
}

// Velocity Verlet, kick-drift-kick.
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

// World metres -> screen pixels. The y axis is subtracted because the screen's
// origin is top-left with y pointing DOWN, while the simulation has y pointing
// up. Both axes use the same scale, or a circular orbit would render as an
// ellipse.
Vector2 toScreen(const Vec2& worldPos) {
    return {
        (float)( worldPos.x * SCALE + WINDOW_W / 2.0 ),
        (float)( WINDOW_H / 2.0 - worldPos.y * SCALE )
    };
}

void render(const std::vector<Particle>& particles,
            const std::deque<Vec2>& trail,
            double simTime, double energy, double e0)
{
    BeginDrawing();
    ClearBackground(BLACK);

    // Trail first, so the bodies draw on top of it.
    for (const Vec2& t : trail) {
        DrawPixelV(toScreen(t), Color{80, 80, 110, 255});
    }

    DrawCircleV(toScreen(particles[0].position), 10.0f, SKYBLUE);      // Earth
    DrawCircleV(toScreen(particles[1].position),  4.0f, LIGHTGRAY);    // Moon

    DrawText(TextFormat("day  %.2f", simTime / 86400.0), 10, 10, 20, RAYWHITE);
    DrawText(TextFormat("E    %.9e", energy),            10, 35, 20, GRAY);
    DrawText(TextFormat("dE/E %.2e", (energy - e0) / std::fabs(e0)), 10, 60, 20, GRAY);
    DrawText(TextFormat("fps  %d", GetFPS()),            10, 85, 20, GRAY);

    DrawText("bodies not to scale - the Moon would be one pixel",
             10, WINDOW_H - 28, 16, DARKGRAY);

    EndDrawing();
}

int main() {
    const double dt = 100.0;          // the value verified against Kepler

    std::vector<Particle> particles;
    // Earth: physical radius, not a drawing size
    particles.push_back({{0, 0},       {0,  -12.44},  {0,0}, 5.972e24, 6.371e6});
    // Moon
    particles.push_back({{ORBIT_R, 0}, {0, 1012.09},  {0,0}, 7.342e22, 1.737e6});

    computeFs(particles);             // so the first half-kick has real values
    const double e0 = totalEnergy(particles);

    std::deque<Vec2> trail;
    double accumulator = 0.0;
    double simTime     = 0.0;

    InitWindow(WINDOW_W, WINDOW_H, "earth-moon");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Frame time is scaled up so an orbit takes seconds rather than weeks.
        // dt itself stays fixed - the physics is unchanged, only how much of it
        // happens per frame.
        accumulator += GetFrameTime() * TIME_SCALE;

        // Cap the catch-up. Without this, dragging the window banks a huge
        // accumulator and the next frame tries to run millions of steps.
        if (accumulator > dt * 500.0) accumulator = dt * 500.0;

        while (accumulator >= dt) {
            step(particles, dt);
            accumulator -= dt;
            simTime     += dt;
        }

        trail.push_back(particles[1].position);
        if (trail.size() > TRAIL_MAX) trail.pop_front();

        render(particles, trail, simTime, totalEnergy(particles), e0);
    }

    CloseWindow();
    return 0;
}