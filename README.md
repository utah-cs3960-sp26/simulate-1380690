# 2D Physics Simulator

A 2D physics simulator built with C++17 and SDL3 that simulates ~1000 circular balls falling through a funnel into a container with gravity, ball-ball collisions, and ball-wall collisions with configurable restitution. Supports CSV scene loading for an image-reveal effect.

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
- **Funnel scene** — V-shaped funnel at top feeds into rectangular box below
- **Per-ball RGB colors** — random pastel or loaded from CSV
- **Gravity** at 980 px/s²
- **Configurable restitution** (0.0–1.0), default 0.5
- **Wall segments** with point-to-segment collision detection
- **Spatial hash grid** for O(n) broad-phase collision detection
- **Fixed timestep** (1/120s) with accumulator, max 8 substeps per frame
- **10 solver iterations** per substep for stable stacking
- **Sleep system** — balls sleep when speed < 5.0 px/s, wake on collision
- **Restitution cutoff** — contacts with relative velocity < 1.0 use e=0
- **Velocity damping** (0.995/substep)
- **CSV scene load/save**
- **HUD** showing FPS, ball count, restitution value, and pause state

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
- `src/simulation.h` / `src/simulation.cpp` — Physics world (balls, wall segments, spatial hash, step, CSV I/O)
- `src/renderer.h` / `src/renderer.cpp` — SDL3 rendering (filled circles, thick wall lines, HUD)
- `tools/assign_colors.py` — Python tool to assign colors from a target image based on final ball positions
