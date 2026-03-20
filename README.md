# 2D Physics Simulator

A 2D physics simulator built with C++17 and SDL3 that simulates ~1000 circular balls inside a walled container with gravity, ball-ball collisions, and ball-wall collisions with configurable restitution.

## Building

### Prerequisites
- macOS with Homebrew
- SDL3: `brew install sdl3`
- CMake: `brew install cmake`

### Build
```bash
mkdir -p build && cd build && cmake .. && make -j$(sysctl -n hw.logicalcpu)
```

### Run
```bash
./build/physics_sim
```

## Controls

| Key | Action |
|-----|--------|
| `Q` / `Escape` | Quit |
| `R` | Reset scene |
| `Space` | Pause / Unpause |
| `+` / `=` | Increase restitution by 0.05 |
| `-` | Decrease restitution by 0.05 |

## Features

- **1000 balls** with varying radii (4–8 px), mass proportional to area
- **Gravity** at 500 px/s²
- **Configurable restitution** (0.0 = inelastic, 1.0 = elastic), default 0.5
- **Rectangular container** with 4 walls plus an interior funnel
- **Spatial hash grid** for O(n) broad-phase collision detection
- **Fixed timestep** (1/120s) with accumulator for frame-rate independence
- **10 solver iterations** per substep for stable stacking
- **Restitution bias** — low-velocity contacts treated as inelastic to prevent jitter
- **Positional correction with slop** — prevents tunneling while allowing stable rest
- **Velocity damping** — balls moving very slowly are damped to reach rest
- **HUD** showing FPS, ball count, restitution value, and pause state

## Architecture

- `src/main.cpp` — SDL3 init, event loop, main loop
- `src/simulation.h` / `src/simulation.cpp` — Physics world (balls, walls, spatial hash, step)
- `src/renderer.h` / `src/renderer.cpp` — SDL3 rendering (circles, thick lines, HUD)

## Design Notes

- Physics uses a semi-implicit Euler integrator: gravity → velocity, then velocity → position, then constraint solving
- Collision resolution is iterative (10 passes) to handle stacking and clusters
- Spatial hash uses a flat 2D grid with cell size 20px, checking each cell against its 4 forward neighbors to avoid duplicate pair checks
- Wall collisions use closest-point-on-segment distance; ball-ball uses center distance vs sum of radii
- Restitution bias threshold of 5.0 units/s kills micro-bounces at rest without affecting real bounces
