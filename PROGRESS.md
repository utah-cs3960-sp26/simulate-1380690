# Progress Log

## Session 10 — Funnel + Wall Segments + Physics Tuning

### Changes
Rewrote simulation to match the full prompt requirements:

1. **Funnel scene layout** — V-shaped funnel at top (two angled walls) feeding into a wide rectangular box below. Balls spawn above the funnel mouth in a grid and fall through.
2. **Wall segments** — All walls are now line segments with proper point-to-segment projection collision. Replaced AABB wall checks.
3. **Thick wall rendering** — Walls drawn as 3px thick lines using SDL_RenderGeometry triangles.
4. **Physics parameters updated**:
   - Gravity: 980 px/s² (was 800)
   - Damping: 0.995/substep (was 0.99)
   - Sleep speed: 5.0 px/s (was 2.0)
5. **Sleep/wake system** — Balls set `sleeping=true` when speed < 5.0. Sleeping balls skip gravity/integration. Woken when hit by another ball.
6. **Ball-ball impulse every iteration** — Velocity response applied on every solver iteration (was only iter==0).
7. **Black background** (was dark blue).
8. **Pastel random colors** (range 150–255).

### Files Modified
- `src/simulation.h` — Added WallSegment struct, sleeping flag on Ball, updated constants
- `src/simulation.cpp` — Full rewrite: wall segments, segment collision, funnel layout, sleep/wake
- `src/renderer.cpp` — Black background, thick line drawing for wall segments

### Tested
- Compiles cleanly
- All controls work (Q/Esc quit, R reset, Space pause, +/- restitution, S save CSV)
- `--scene` CSV loading preserved

## Session 9 — Complete Rewrite (Clean Implementation)

### Problem
Previous implementation (sessions 1-8) had persistent vortex/tunnel behavior where balls would never settle. Over-engineering with funnel walls, complex sleep/wake systems, swept collision detection, and excessive tuning parameters made the physics unstable.

### Approach
Deleted all previous source code and started fresh with the simplest correct approach:

1. **Simple rectangular container** — 4 axis-aligned walls (left, right, top, bottom), no funnel
2. **Straightforward physics loop per substep**:
   - Apply gravity: `vy += gravity * dt`
   - Apply damping: `vel *= 0.99`
   - Move balls: `pos += vel * dt`
   - 10 solver iterations resolving wall + ball-ball collisions
   - Sleep: zero velocity if speed < 2.0
3. **Simple wall collisions** — direct AABB checks against 4 walls, push out + reflect with restitution
4. **Simple ball-ball collisions** — push apart by full overlap, momentum exchange with restitution
5. **Low-velocity restitution cutoff** — if relative collision velocity < 1.0, use e=0 (no bounce)
6. **Spatial hash grid** — flat counting-sort grid (cell size 16px) for O(n) broad phase
7. **Fixed timestep** — 1/120s, max 8 substeps per frame
8. **Gravity = 800 px/s²** — moderate, not excessive

### Files
- `src/simulation.h` — Ball, World structs (simplified)
- `src/simulation.cpp` — physics (rewritten from scratch)
- `src/renderer.h/.cpp` — drawing (simplified, removed thick_line for walls)
- `src/main.cpp` — SDL3 init, main loop, input (unchanged logic)

### Tested
- Compiles cleanly with no errors
- Runs for 3+ seconds without crashing
- 1000 balls generated, initial.csv auto-saved correctly
- Controls: Q/Esc quit, R reset, Space pause, +/- restitution, S save CSV
- `--scene` CSV loading preserved
