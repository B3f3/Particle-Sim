# Particle Sim

A 2D gravitational N-body simulator in C++17, written from scratch. Velocity
Verlet integration, an O(n²) force solver, and live rendering with raylib.

Verified against Kepler's third law: it reproduces the Earth–Moon orbital period
to **1.9 parts per million**.

---

## Results

### Earth–Moon, circular orbit

Real masses and distances in SI units, integrated with velocity Verlet at
`dt = 100 s` over 3 million seconds (~1.27 orbits).

| quantity | value |
|---|---|
| measured period | 2,357,434.649 s = **27.2851232 days** |
| theoretical period | 2,357,430.162 s = 27.2850713 days |
| **error** | **0.00019 %** |
| eccentricity achieved | 0.00013 |
| relative energy drift | 7.1 × 10⁻⁸ (bounded, not growing) |

The real sidereal month is 27.3217 days. The remaining 0.13 % is solar
perturbation and the Moon's actual eccentricity of 0.055 — physics this
simulation deliberately doesn't model, not integrator error.

### Earth–Moon, elliptical orbit

The same code with initial conditions 1 % above circular velocity, producing an
ellipse of eccentricity 0.0198 and semi-major axis 392,147,126 m:

| quantity | value |
|---|---|
| measured period | 2,429,138.30 s |
| theoretical period for that semi-major axis | 2,429,054.85 s |
| **error** | **0.0034 %** |

Matching the period of a non-trivial ellipse is the stronger result — anything
can hit a circle.

### Integrator comparison

The same two-body configuration run with each integrator, tracking total energy:

| integrator | behaviour over the run |
|---|---|
| explicit Euler | climbs monotonically, never returns |
| semi-implicit Euler | climbs, then **turns over** — first quarter of a bounded oscillation |
| velocity Verlet | bounded oscillation, two orders of magnitude tighter |

The difference between the first two is a single statement reordered: updating
velocity before position instead of after. That one move changes the method from
energy-divergent to symplectic.

---

## Build and run

Requires raylib. On Debian-family systems it may not be packaged; building from
source takes a couple of minutes (pass `-DBUILD_EXAMPLES=OFF`, since the demo
programs fail to link on some setups).

```bash
make && ./sim
```

The `Makefile`:

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS     = -lraylib -lm -lpthread -ldl -lrt -lX11
```

For headless verification runs, drop the raylib libraries and comment out the
render calls — the physics has no dependency on the renderer.

---

## How it works

**`Vec2`** — a header-only `double`-precision 2D vector with operator
overloading. Lives in `headers/Vec2.h`, deliberately *not* named `Vector2`,
because raylib defines its own float-based `Vector2` and the two would collide.

**`Particle`** — position, velocity, acceleration, mass, radius. Acceleration is
stored rather than local because Verlet needs it to persist between steps.

**`computeFs()`** — zeroes all accelerations, then iterates over **pairs**
(`j = i + 1`), applying each computed acceleration with `+=` to one particle and
`-=` to the other. One value, opposite signs, so Newton's third law holds
exactly rather than approximately.

**`step()`** — velocity Verlet in kick-drift-kick form:

```
half-kick   velocity += acceleration * dt/2     (acceleration from last step)
drift       position += velocity * dt
forces      recompute at the new positions
half-kick   velocity += acceleration * dt/2     (new acceleration)
```

This form needs no saved copy of the old acceleration: it's used in the first
half-kick and finished with before `computeFs` overwrites it.

**Softening** — the force law uses `G·M / (r² + ε²)` rather than `G·M / r²`.
Without it a close encounter sends acceleration toward infinity and launches a
particle out of the simulation. With ε ≈ 1 % of typical separation, the error is
0.01 % at orbital distance and only becomes significant when bodies nearly
coincide.

---

## Verification

A gravitational simulation looks convincing whether or not it is correct. A
subtly wrong integrator produces orbits that are visually perfect for hundreds
of frames and then quietly diverge. So the physics was verified numerically
before anything was drawn — the window came last.

Three independent checks:

**Kepler's third law.** `T = 2π√(r³/GM)` gives the period analytically. Measured
against it by tracking the unwrapped angle of the separation vector and
interpolating the crossing. This is the primary verdict.

**Energy conservation.** `KE + PE` must be constant. How it fails is diagnostic:
monotonic growth means the integrator is injecting energy, and the rate reveals
the method's order.

**Momentum conservation.** `Σ mᵢvᵢ` must be constant to floating-point precision
regardless of integrator, because it follows from the forces being equal and
opposite rather than from how they're integrated.

The pair of them isolates different halves of the code:

- **momentum drifts** → the force loop is wrong
- **energy drifts, momentum holds** → the integrator is wrong

Every run writes a CSV with per-particle state plus both conserved quantities,
at `setprecision(15)` — the default six significant figures is enough to hide
the drift entirely.

### Two things worth recording

**The diagnostic can be wrong while the simulation is right.** An early run
showed energy *decreasing* under explicit Euler, which contradicts theory. The
cause was a mismatch: the force used the softened law while `totalEnergy`
computed the unsoftened potential `−GMm/r`. Softened gravity is a different
force law with its own potential, so the diagnostic was measuring the energy of
a system that wasn't being simulated. Once both used `√(r² + ε²)`, energy
climbed as it should. The lesson: a plausible wrong answer from a trusted
diagnostic is more dangerous than an obvious failure.

**Judge conservation relative to scale.** A momentum column reading −20,971,520
looks alarming until you notice the momentum scale is 7.5 × 10²⁵, making that
2.8 × 10⁻¹⁹ of it — the arithmetic showing its own precision floor, since the
value is the difference of two near-equal numbers around 2 × 10¹⁸.

`double` throughout, not `float`. Position accumulates small increments over
hundreds of thousands of steps; float's ~7 significant digits would produce
drift comparable in size to the effects being measured.

---

## Rendering

Fixed-timestep accumulator, decoupled from frame rate — a stutter must not alter
the physics, or runs stop being reproducible and two integrators can't be
compared on identical conditions. A `TIME_SCALE` multiplier compresses a
27-day orbit into about ten seconds of wall clock while `dt` stays at the
verified value.

World-to-screen uses the same scale on both axes (different scales render a
circular orbit as an ellipse) and subtracts on y, since the screen origin is
top-left with y pointing down.

Bodies are drawn at fixed pixel sizes and labelled as not to scale. At a scale
where the Moon's orbit spans 300 pixels, Earth is 5 px and the Moon is **1 px** —
the Earth–Moon system is overwhelmingly empty space, which is why every
astronomy diagram lies about body sizes.

A trailing buffer of recent Moon positions renders the orbit itself, which makes
integrator failure visible rather than merely measurable: under explicit Euler
the trail spirals outward instead of closing.

---

## Not implemented

- Collision detection and impulse response
- Barnes-Hut — the O(n²) solver is comfortable to roughly 2,000 particles and
  hopeless beyond ~10,000
- Multithreading, which would be straightforward since the pair loop is
  embarrassingly parallel
- 3D, rotation, friction, constraints
