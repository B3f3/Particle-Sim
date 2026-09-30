# Particle Sim — Cheat Sheet

Reference for the 10-task particle simulator. Keep it next to the brief.

---

## ⚠ Name collision: raylib already has a `Vector2`

`raylib.h` defines its own:

```c
typedef struct Vector2 { float x; float y; } Vector2;
```

Include that and your own `Vector2.h` in the same file and you get a
redefinition error — and if you dodge it by using raylib's, you silently get
**float** precision in your integrator, which is the one thing the brief warns
against.

**Fix:** name yours something else. `Vec2` is the usual choice.

```cpp
struct Vec2 { double x, y; /* ... */ };
```

Then convert only at the drawing call, where float is fine:

```cpp
DrawCircle((int)screenX, (int)screenY, (float)p.radius, BLUE);
```

Alternatively put yours in a namespace (`namespace sim { struct Vector2 ... }`)
and refer to `sim::Vector2`. Renaming is less typing.

---

## Build and run

```bash
make && ./sim
```

`Makefile` (the indented lines are TABS, not spaces):

```makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
LIBS     = -lraylib -lm -lpthread -ldl -lrt -lX11

sim: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o sim $(LIBS)

clean:
	rm -f sim
```

Add `-O2` to `CXXFLAGS` once correctness is settled — it typically gives a
5–10× speedup on the O(n²) force loop. Keep `-g` alongside it so you can still
debug.

Headless mode (for tasks 2–6, no window): same command minus the raylib libs.

---

## Physics formulas

With **G = 1**, masses 1–1000, distances 1–1000.

### Gravity

```
d   = p2.position - p1.position
r²  = d.lengthSquared()
a1  = d.normalise() * (G * m2 / (r² + ε²))
```

`m2` alone, not `m1*m2` — dividing force by `m1` cancels it.
`ε` ≈ 1% of typical separation, to stop close encounters exploding.

### Circular orbit setup

```
v = sqrt(G * M / r)        perpendicular to the radius
```

### Kepler's third law

```
T = 2π * sqrt(r³ / (G * M))
```

### Energy

```
KE = Σ  0.5 * mᵢ * vᵢ.lengthSquared()
PE = − Σ  G * mᵢ * mⱼ / rᵢⱼ          over pairs i < j, ONCE each
E  = KE + PE                          must be constant
```

PE is negative. Summing ordered pairs double-counts it.

### Momentum

```
p = Σ mᵢ * vᵢ                         must be constant, exactly
```

### Integrators

**Euler** (task 4 — wrong on purpose):
```
position += velocity * dt
velocity += acceleration * dt
```

**Semi-implicit Euler** (symplectic, one-line fix):
```
velocity += acceleration * dt
position += velocity * dt              // the NEW velocity
```

**Velocity Verlet** (the one to keep):
```
position += velocity * dt + acceleration * (0.5 * dt * dt)
newAcc    = computeForces()
velocity += (acceleration + newAcc) * (0.5 * dt)
acceleration = newAcc
```

### Collision

Detection (no sqrt):
```
d.lengthSquared() < (r1 + r2) * (r1 + r2)
```

Response:
```
n      = (p2.position - p1.position).normalise()
vRel   = p1.velocity - p2.velocity
vAlong = vRel.dot(n)

if (vAlong > 0) return;                // already separating — skip

j = -(1 + e) * vAlong / (1/m1 + 1/m2)

p1.velocity += n * (j / m1)
p2.velocity -= n * (j / m2)
```

`e` = restitution: 1 elastic, 0 perfectly inelastic.

---

## Verification numbers

Know these before you run anything.

| Check | Expected |
|---|---|
| `Vec2{3,4}.length()` | exactly 5 |
| `Vec2{3,4}.normalise().length()` | 1 |
| `(v - v)` | {0, 0} |
| Straight line: v=(1,0), dt=0.01, 100 steps | x = 1.0 (not 0.99, not 1.01) |
| Circular velocity, G=1 M=1000 r=100 | v = √10 ≈ **3.16228** |
| Orbital period, same setup | T ≈ **198.69** |
| Total momentum, any time, any integrator | constant to ~1e-15 |
| Total energy, Verlet | constant to a small oscillation |
| Total energy, plain Euler | **climbs** — that's the lesson |
| Equal masses, head-on, e=1, v = +1 and −1 | they swap: −1 and +1 |

**The two diagnostics isolate different halves of the code:**

- momentum drifts → the **force loop** is wrong (not equal and opposite)
- energy drifts, momentum fine → the **integrator** is wrong

---

## raylib essentials

The ~15 functions this project needs, out of several hundred.

```cpp
#include "raylib.h"

InitWindow(800, 600, "particle sim");
SetTargetFPS(60);

while (!WindowShouldClose()) {          // Esc or window close button
    // physics steps here
    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawCircle(x, y, radius, BLUE);          // ints for x,y; float radius
        DrawLine(x1, y1, x2, y2, GRAY);
        DrawText("hello", 10, 10, 20, DARKGRAY); // x, y, fontSize, colour
    EndDrawing();
}

CloseWindow();
```

**`TextFormat` is printf for `DrawText`** — this is the one that makes the
on-screen readout easy:

```cpp
DrawText(TextFormat("E = %.6f", energy), 10, 10, 20, DARKGRAY);
DrawText(TextFormat("n = %d  fps = %d", count, GetFPS()), 10, 35, 20, DARKGRAY);
```

Colours: `RAYWHITE BLACK WHITE RED GREEN BLUE YELLOW ORANGE PURPLE
DARKGRAY LIGHTGRAY SKYBLUE MAROON`. Custom: `Color{r, g, b, 255}`.

Input: `IsKeyPressed(KEY_SPACE)` (once per press), `IsKeyDown(KEY_RIGHT)`
(every frame while held), `GetMousePosition()`, `GetMouseWheelMove()`.

Useful keys: `KEY_SPACE KEY_R KEY_ESCAPE KEY_UP KEY_DOWN KEY_ONE`…

`GetFrameTime()` returns seconds since last frame as a float — for the
accumulator, **not** as your physics `dt`.

---

## World → screen transform

Screen origin is top-left with **y pointing down**; your world is centred on
zero with y pointing up. Hence the subtraction:

```cpp
int screenX = (int)( worldX * scale + screenWidth  / 2.0 );
int screenY = (int)( screenHeight / 2.0 - worldY * scale );
```

Forget the flip and everything runs correctly but appears mirrored vertically —
confusing precisely because it doesn't look broken.

Use the **same `scale` for both axes**, or circular orbits render as ellipses.

---

## Fixed timestep

```cpp
const double dt = 0.01;
double accumulator = 0.0;

while (!WindowShouldClose()) {
    accumulator += GetFrameTime();
    while (accumulator >= dt) {
        step(particles, dt);
        accumulator -= dt;
    }
    render(particles);
}
```

Never use `GetFrameTime()` as `dt` directly — a stutter would change the physics
and make runs non-reproducible, which destroys your ability to compare two
integrators on identical initial conditions.

Cap the inner loop (`if (accumulator > 0.25) accumulator = 0.25;`) or a long
stall sends it into a spiral of ever-more catch-up steps.

---

## Debugging

**Address sanitizer** — catches out-of-bounds and use-after-free with exact
line numbers. Costs ~2× runtime, worth it while developing:

```bash
g++ -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined main.cpp -o sim $(LIBS)
```

**NaN check** — the failure mode this project produces most. NaN propagates
silently through every subsequent operation, so catch it at the step where it
appears:

```cpp
#include <cmath>
if (std::isnan(p.position.x) || std::isnan(p.position.y)) {
    std::cerr << "NaN at step " << stepCount << " particle " << i << "\n";
    std::exit(1);
}
```

Sources of NaN here: normalising a zero vector, `0/0` in the force law when two
particles coincide, `sqrt` of a negative.

**Status line that overwrites itself** rather than scrolling:

```cpp
std::cout << "\rt=" << t << "  E=" << E << "  p=" << p << std::flush;
```

`std::flush` is needed — without a newline the stream won't flush on its own.

**Printing enough digits.** Default `cout` gives 6 significant figures, which
will hide the drift you're looking for:

```cpp
std::cout << std::setprecision(15);   // needs <iomanip>
```

**gdb**, if a crash needs it:

```bash
gdb ./sim
run
bt                  # backtrace — where it died
print particles[3]  # inspect a variable
```

---

## C++ gotchas hit in this project

**Narrowing in braced returns.** `return {a, b};` where the expression is
`double` and the member is `float` is ill-formed — a hard error, not a warning.
Keeping everything `double` avoids it.

**const-correctness.** Any member that doesn't modify the object needs `const`,
or you can't call it on the `const Particle&` references the force loop uses.
They're all-or-nothing: a `const` method can only call other `const` methods.

**`+=` returns a reference.** `Vec2& operator+=(...)` with `return *this;`, and
no `const` — it modifies. A `const` version that returns a new value compiles
and silently does nothing.

**Scalar params by value**, not `const double&`. Referencing an 8-byte value
costs more than copying it.

**`pow(x, 2)` is slow** — goes through logarithms. `x * x` is one multiply.

**`std::sort` is not stable.** Irrelevant here, but if you ever sort particles
and need reproducible ordering across runs, use `std::stable_sort` or break ties
explicitly.

---

## Scope reminders

O(n²) force loop is fine to ~2000 particles. Beyond that needs Barnes-Hut,
which is a separate project.

Tunnelling: fast particles can pass through each other between steps. Smaller
`dt` mitigates it; continuous collision detection fixes it properly and is out
of scope.

Not in scope: 3D, rotation/angular momentum, friction, springs, constraints,
GPU, relativity.
