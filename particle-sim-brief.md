# Project Brief: Particle Simulator

A 2D simulation of particles with mass, mutual gravitational attraction, and
elastic collisions, rendered in a window. C++, SDL2 for the display, nothing
else.

Ten tasks. The physics is verified numerically before anything is drawn, because
an animated simulation looks convincing whether or not it is correct — a subtly
wrong integrator produces orbits that are beautiful and wrong, and you cannot
see the difference. The numbers can.

## Units

Do not use SI. `G = 6.674e-11` with kilograms and metres gives you numbers
spanning thirty orders of magnitude and nothing to eyeball.

Set **G = 1**, masses in the range 1–1000, distances in the range 1–1000. The
physics is identical, the numbers stay readable, and a circular orbit at r = 100
around a mass of 1000 has a period you can compute in your head.

---

## Task 1 — Vector2

A struct holding `double x, y` with the operations you'll need: `+`, `-`,
multiplication by a scalar, `length()`, `lengthSquared()`, and `normalized()`.

Everything downstream is vector arithmetic, and writing it inline with separate
`x` and `y` lines doubles the length of every formula and doubles the places a
sign error can hide.

**Check:** `Vector2(3, 4).length()` is exactly 5. `normalized()` of that has
length 1. A vector minus itself is zero.

**Note:** keep `lengthSquared()` as its own function. Gravity needs r² and
collision detection needs a distance comparison, and both can avoid the square
root entirely — `sqrt` is the most expensive thing in your inner loop.

---

## Task 2 — Particle and the simulation loop

```cpp
struct Particle {
    Vector2 position;
    Vector2 velocity;
    Vector2 acceleration;
    double mass;
    double radius;
};
```

Hold them in a `std::vector<Particle>`. Write a loop that advances time by a
fixed `dt` for a fixed number of steps, and writes every particle's position to
a CSV file each step — or every tenth step, so the file stays manageable.

No forces yet. Give one particle a constant velocity and confirm it moves in a
straight line at the expected rate.

**Check:** a particle at (0,0) with velocity (1,0) and dt = 0.01 is at x = 1.0
after 100 steps. If it's at 0.99 or 1.01, your loop is off by one step.

**Why CSV:** you can plot it, check it, and diff two runs against each other.
This file is your instrument for the next four tasks.

---

## Task 3 — Gravitational force between two bodies

Newton's law of universal gravitation:

```
F = G · m₁ · m₂ / r²
```

directed along the line joining the two particles. In vector form, the
acceleration of particle 1 due to particle 2 is:

```
d     = p₂.position - p₁.position
r²    = d.lengthSquared()
a₁    = d.normalized() * (G · m₂ / r²)
```

Note `m₂` alone, not `m₁ · m₂` — the force divided by `m₁` gives acceleration,
and `m₁` cancels. That cancellation is why all objects fall at the same rate,
and getting it wrong is a common early bug that makes heavy particles accelerate
too slowly.

**Softening.** As r approaches zero the force approaches infinity, and one
close encounter flings a particle out of the simulation at implausible speed.
The standard fix is a softening length ε:

```
a = d.normalized() * (G · m₂ / (r² + ε²))
```

ε around 1% of your typical separation. This is not a hack — every professional
N-body code does it, because point masses are an idealisation and real bodies
have extent.

**Check:** two equal masses released from rest should accelerate toward each
other symmetrically and meet at their midpoint. If the midpoint drifts, your
force isn't equal and opposite.

---

## Task 4 — Euler integration, and an orbit that fails

The obvious integrator:

```
position += velocity * dt
velocity += acceleration * dt
```

Set up a circular orbit to test it. For a light particle orbiting a heavy one at
distance r, the circular velocity is:

```
v = sqrt(G · M / r)
```

directed perpendicular to the line joining them. With G = 1, M = 1000 and
r = 100, that's v = sqrt(10) ≈ 3.162.

Run it and plot the CSV.

**Check:** Kepler's third law gives the period analytically:

```
T = 2π · sqrt(r³ / (G · M))
```

For those numbers, T ≈ 198.7. Your orbit should return to its starting position
after that much simulated time.

**It won't.** The orbit will spiral outward, and the further it goes the worse
it gets. This is the point of the task. Do not skip ahead and fix it — get it
wrong first, then measure how wrong in the next task.

---

## Task 5 — Energy conservation, and why Euler fails

Compute the total energy every step and print it:

```
KE = Σ  ½ · mᵢ · vᵢ²
PE = − Σ  G · mᵢ · mⱼ / rᵢⱼ        (over each pair once, i < j)
E  = KE + PE
```

Potential energy is negative, and the sum runs over each *pair*, not each
ordered pair — count a pair twice and your total is wrong by a factor of two.

Total energy must be constant. Run your Euler orbit and watch it climb.

**The reason:** Euler's method systematically injects energy into oscillatory
systems. It isn't an accuracy problem — halving `dt` halves the rate of drift
but never removes it, so you cannot fix this by taking smaller steps. The error
is in the structure of the method, not its precision.

**Two fixes, one of them free.**

*Semi-implicit Euler* is a one-line change — update velocity **first**, then use
the new velocity to update position:

```
velocity += acceleration * dt
position += velocity * dt        // new velocity, not old
```

That reordering makes the method symplectic: energy now oscillates around the
correct value instead of climbing away from it. Same cost, completely different
long-term behaviour.

*Velocity Verlet* is second-order accurate and the standard choice:

```
position += velocity * dt + acceleration * 0.5 * dt²
newAcceleration = computeForces()
velocity += (acceleration + newAcceleration) * 0.5 * dt
acceleration = newAcceleration
```

It needs the old acceleration kept between steps, which is why `Particle` has an
`acceleration` member.

**Check:** implement both, run the same orbit with each, and plot energy against
time for all three integrators on one graph. Euler climbs, semi-implicit
oscillates in a bounded band, Verlet oscillates in a much tighter band. That
graph is the single most interesting artefact this project produces and it
belongs in the README.

**Then re-check Kepler.** With Verlet, your orbital period should match
T ≈ 198.7 to within a fraction of a percent.

---

## Task 6 — N bodies

Generalise the force calculation to every pair. The straightforward version is
two nested loops, O(n²):

```
for i in particles:
    a[i] = 0
for i, j in pairs (i < j):
    compute the force once
    apply +a to i and −a to j
```

Iterating over pairs rather than all ordered pairs halves the work and
guarantees Newton's third law holds exactly — the forces are equal and opposite
because they're the same computed value with opposite signs. Computing each
direction independently invites a sign or rounding asymmetry.

**Check:** total momentum `Σ mᵢ · vᵢ` must be conserved exactly, to floating
point precision, regardless of what the particles do. Unlike energy, momentum
conservation doesn't depend on the integrator — it follows from the forces being
equal and opposite. So if momentum drifts, your force loop is wrong; if energy
drifts but momentum doesn't, your integrator is wrong. Two independent
diagnostics that isolate different halves of the code.

**Scale note:** O(n²) is fine to about 2000 particles. Beyond that you need a
Barnes-Hut tree, which is a project in itself.

---

## Task 7 — The window

Now the physics is verified, put it on screen.

```bash
sudo apt install libsdl2-dev
g++ -std=c++17 -Wall -Wextra main.cpp -o sim $(sdl2-config --cflags --libs)
```

Minimum viable: `SDL_CreateWindow`, `SDL_CreateRenderer`, and a loop that polls
events (so the close button works), clears the screen, draws each particle, and
presents. Draw particles as filled rectangles first — `SDL_RenderFillRect` is
one call. Circles need a helper and can wait.

**World coordinates are not screen coordinates.** Your simulation works in
arbitrary units centred on zero; the screen is 800×600 with the origin at the
top-left and y pointing *down*. You need a transform, and the y-flip is the part
everyone forgets — the symptom is a simulation that runs correctly but appears
mirrored vertically.

**Decouple physics from frame rate.** Do not use the frame time as `dt`. A
stutter would then change the physics, making the simulation non-deterministic
and unrepeatable. Use a fixed `dt` and run however many physics steps are needed
per frame:

```
accumulator += frameTime
while (accumulator >= dt) {
    step(dt);
    accumulator -= dt;
}
render();
```

**Check:** the orbit you verified in task 5 should now be visibly circular and
should close on itself rather than drifting. If it looks like an ellipse that
slowly rotates, your rendering transform has different x and y scales.

---

## Task 8 — Collision detection

Two circles overlap when the distance between their centres is less than the sum
of their radii:

```
distanceSquared < (r₁ + r₂)²
```

Compare squared values — no square root needed, and this runs for every pair
every step.

For now, just detect and print. Set up two particles on a collision course and
confirm it fires exactly once as they meet.

**Gotcha:** with a large `dt` and fast particles, two particles can pass
completely through each other between steps — they're apart before, apart after,
never overlapping when you check. This is *tunnelling*, and the cheap answer is
a smaller `dt`. The proper answer is continuous collision detection, which is
out of scope. Worth knowing the failure exists so you recognise it when
particles occasionally ignore each other.

---

## Task 9 — Collision response

When two particles collide, exchange momentum along the line joining their
centres. The component of motion perpendicular to that line is unaffected.

```
n      = (p₂.position − p₁.position).normalized()
vRel   = p₁.velocity − p₂.velocity
vAlong = vRel · n                                  (dot product)

if vAlong > 0: skip — they're already separating

j = −(1 + e) · vAlong / (1/m₁ + 1/m₂)

p₁.velocity += n * (j / m₁)
p₂.velocity −= n * (j / m₂)
```

`e` is the coefficient of restitution: 1 for perfectly elastic, 0 for perfectly
inelastic (they move together), anything between for realistic bouncing.

**The separating check matters.** Without it, particles that are still
overlapping on the next step get a second impulse, and they stick together
vibrating. This is the bug that makes collision code feel haunted.

**Positional correction.** An impulse fixes velocities but not the overlap that
already happened. Push the particles apart along `n`, splitting the correction
in inverse proportion to mass, so a light particle moves most of the way and a
heavy one barely shifts.

**Checks, and these are exact:**

- Total momentum is conserved by every collision, for any `e`. To floating point
  precision. No exceptions.
- With `e = 1`, total kinetic energy is conserved too.
- Two **equal masses**, head-on, `e = 1`: they exactly swap velocities. Set up
  +1 and −1, run it, and confirm you get −1 and +1. That's a hand-checkable case
  with a known answer.
- A light particle striking a stationary heavy one bounces back at nearly its
  incoming speed, and the heavy one barely moves.

---

## Task 10 — Make something with it

The machinery is done; now use it.

- **A solar system.** One heavy central mass, several orbiting bodies at
  different radii, each given its circular velocity. Verify the outer ones have
  longer periods and check the ratios against Kepler's third law: T² ∝ r³.
- **A collapsing cloud.** A few hundred particles at rest in a disc, released.
  They collapse, and the collisions and close encounters produce structure.
- **A binary system** with a third body — the three-body problem, which has no
  closed-form solution and is genuinely chaotic. Run it twice with one position
  differing in the twelfth decimal place and watch the trajectories diverge.
  That's a real demonstration of sensitive dependence on initial conditions, and
  it's a good thing to have produced yourself.

---

## Scope

Done means: a window showing a stable multi-body system with collisions, where
energy and momentum are conserved to a documented tolerance.

Not in scope: 3D, Barnes-Hut, rotation or angular momentum of bodies, friction,
springs, constraints, GPU acceleration, relativity.

## Where the difficulty lies

Not in the physics — every formula you need is in this document.

It's that **nothing crashes when it's wrong**. A wrong force law gives you a
simulation that runs happily and looks plausible. A wrong integrator gives you
orbits that are visually perfect for the first hundred steps. The only way to
know is to compute a quantity you can check against a number that was known
before you started, which is why tasks 4, 5 and 6 come before the window and why
the checks are specific numbers rather than "looks right".

That's a different discipline from everything you've built so far, where `cmp`
or `diff` gave you a verdict. Here you have to construct the verdict yourself.
