# 2D Physics Simulator

A 2D physics simulator built with C++17 and SDL3 that simulates ~1000 circular balls inside a walled container with gravity, ball-ball collisions, and ball-wall collisions with configurable restitution. Supports CSV scene loading for an image-reveal effect.

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
./build/physics_sim                       # default random scene
./build/physics_sim --scene initial.csv   # load scene from CSV
```

## Controls

| Key | Action |
|-----|--------|
| `Q` / `Escape` | Quit |
| `R` | Reset scene |
| `Space` | Pause / Unpause |
| `+` / `=` | Increase restitution by 0.05 |
| `-` | Decrease restitution by 0.05 |
| `S` | Save ball positions to `final_positions.csv` |

## Features

- **1000 balls** with varying radii (4–6 px), mass proportional to area
- **Per-ball RGB colors** — each ball has its own color
- **Gravity** at 2200 px/s²
- **Configurable restitution** (0.0 = inelastic, 1.0 = elastic), default 0.5
- **Rectangular container** with 4 walls plus an interior funnel (80px opening)
- **Spatial hash grid** for O(n) broad-phase collision detection
- **Fixed timestep** (1/180s) with accumulator for frame-rate independence
- **12 solver iterations** per substep for stable stacking
- **Swept wall collision detection** — prevents tunneling through walls using prev_pos tracking
- **Hysteretic sleep system** — balls must be in contact and slow for 60 frames to sleep; require 3 consecutive unsupported substeps to wake
- **Restitution cutoff** — dynamic threshold based on gravity×dt, low-velocity contacts treated as inelastic
- **Positional correction** — tight push-out for walls (0.05px slop), 60% correction for ball-ball (0.2px slop)
- **Ball-ball tangential friction** — dissipates lateral/rotational energy to prevent vortex circulation
- **Velocity damping** (0.9998 factor) — very subtle energy bleed for settling
- **CSV scene load/save** — load initial positions from CSV, save final positions
- **HUD** showing FPS, ball count, restitution value, sleeping count, and pause state

## Image Reveal Workflow

1. Run the simulator with default scene and let balls settle
2. Press `S` to save `final_positions.csv`
3. Run the color assignment tool:
   ```bash
   python3 tools/assign_colors.py initial.csv final_positions.csv target_image.png output.csv
   ```
4. Run the simulator with the colored CSV:
   ```bash
   ./build/physics_sim --scene output.csv
   ```
5. When balls settle, the target image appears!

### CSV Format
One row per ball: `x, y, radius, color_r, color_g, color_b`

Example: `512, 100, 5, 255, 0, 128`

## Architecture

- `src/main.cpp` — SDL3 init, event loop, CLI argument parsing
- `src/simulation.h` / `src/simulation.cpp` — Physics world (balls, walls, spatial hash, step, CSV I/O)
- `src/renderer.h` / `src/renderer.cpp` — SDL3 rendering (per-ball colored circles, thick lines, HUD)
- `tools/assign_colors.py` — Python tool to assign colors from a target image based on final ball positions

## Design Notes

- Physics uses a semi-implicit Euler integrator: gravity → velocity, then velocity → position, then constraint solving
- Collision resolution is iterative (12 passes) to handle stacking and clusters
- Spatial hash uses a flat 2D grid with cell size 20px, checking each cell against its 4 forward neighbors
- Sleep system: hysteretic — balls must be in contact AND below 12 px/s for 60 substeps to sleep. Waking requires 3 consecutive unsupported substeps, preventing noise-triggered wake.
- Swept wall collision: tracks prev_pos to detect wall crossings even if ball moves fast enough to skip past a wall in one substep.
- Dynamic restitution cutoff: max(20, 2×gravity×dt) ensures resting contacts always use e=0 regardless of gravity tuning
- Ball-ball tangential friction (0.02) dissipates lateral circulation energy to prevent vortex behavior in dense piles
