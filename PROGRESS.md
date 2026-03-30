# Progress Log

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

### What Changed vs Previous
- Removed funnel walls entirely
- Removed sleep/wake state machine (replaced with simple velocity zeroing)
- Removed swept collision detection
- Removed prev_pos tracking
- Removed wall friction, ball-ball tangential friction
- Removed progressive damping, hysteretic wake, contact counting
- Wall collision is now simple AABB instead of point-to-segment
- Gravity reduced from 2200 to 800
- Damping changed from 0.9998/substep to 0.99/substep (more aggressive energy drain)

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
