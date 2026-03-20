# Physics Simulator - Amp Autonomous Build Prompt

## Project Goal
Build a 2D physics simulator using C++ and SDL3 that simulates ~1000 circular balls inside a container made of fixed walls. The simulation should feature gravity, ball-ball collisions, and ball-wall collisions with configurable restitution (inelastic collisions). The simulation must be stable (no tunneling, no infinite vibration/energy explosions) and visually smooth.

## Tech Stack
- Language: C++17 or later
- Graphics/Window: SDL3 (install via Homebrew if not present: `brew install sdl3`)
- Build system: CMake
- Platform: macOS

## Setup Steps (do these first)
1. Check if SDL3 is installed: `brew list sdl3 2>/dev/null || brew install sdl3`
2. Check if CMake is installed: `brew list cmake 2>/dev/null || brew install cmake`
3. Initialize a CMakeLists.txt that links against SDL3
4. Verify the project compiles and a blank SDL3 window opens before adding physics

## Architecture

### Files to create
- `CMakeLists.txt` — build config
- `src/main.cpp` — entry point, SDL3 init, main loop
- `src/simulation.h` / `src/simulation.cpp` — physics world: balls, walls, step()
- `src/renderer.h` / `src/renderer.cpp` — SDL3 rendering of balls and walls
- `README.md` — build instructions, controls, design notes
- `PROGRESS.md` — running log of what has been implemented and what still needs fixing

### Core Data Structures
- `Ball`: position (x,y), velocity (vx,vy), radius, mass (proportional to radius^2)
- `Wall`: two endpoints (line segment), normal vector — **fixed and immovable**
- `World`: list of balls, list of walls, gravity constant, restitution coefficient

## Physics Requirements

### Gravity
- Apply downward acceleration each frame (e.g., 9.8 units/s²), scaled by delta time

### Collision Detection & Response
- **Ball-wall**: signed distance from ball center to wall line; if < radius, resolve penetration and reflect velocity using restitution
- **Ball-ball**: if distance between centers < sum of radii, resolve overlap and exchange momentum using restitution
- Restitution coefficient `e` (0.0 = perfectly inelastic, 1.0 = perfectly elastic) should be a runtime-configurable constant (e.g., defined at top of simulation.cpp or passed via constructor). Default: 0.5
- **Penetration correction**: after collision response, push balls apart positionally so they do not overlap — this is critical to prevent tunneling and vibration explosions
- Use iterative collision resolution: run several solver iterations per frame (e.g., 8–16 iterations) to handle stacking and clusters

### Stability Requirements (very important)
- Balls must NEVER tunnel through walls or other balls
- Balls must NOT gain energy over time — damp residual velocity when speed is very low (sleep threshold)
- With restitution = 0.0, balls should come to rest quickly; with restitution = 0.9, they should bounce a long time but still eventually settle
- Final resting volume/area of the pile should be approximately the same regardless of restitution (same number of balls, same sizes)

### Scene Setup
- Container: a rectangular "box" made of 4 wall segments, plus optionally a funnel or ramp in the middle to make it visually interesting
- ~1000 balls of slightly varying radii (e.g., 4–8 px), placed in a grid or random scatter above the container floor with no initial overlap
- Initial velocity: zero or small random jitter

## Rendering
- Render at 60 fps target using SDL3
- Draw walls as thick white or gray lines
- Draw balls as filled circles (use a simple midpoint circle or SDL_RenderFillCircle approximation)
- Show a simple HUD (SDL_RenderDebugText or a basic overlay) displaying: FPS, ball count, restitution value
- Window size: 1280x720 or 1024x768

## Controls
- `Q` or `Escape`: quit
- `R`: reset scene
- `+` / `-`: increase/decrease restitution by 0.05 (clamped to [0.0, 1.0])
- `Space`: pause/unpause

## Performance
- The simulation must run at interactive frame rates (>= 30 fps) with 1000 balls on a modern Mac
- Use a spatial hash grid or similar broad-phase collision detection to avoid O(n²) ball-ball checks
- Keep the physics timestep fixed (e.g., 1/120s) and allow multiple substeps per render frame if needed

## Testing Protocol (run after every major change)
1. `mkdir -p build && cd build && cmake .. && make -j$(sysctl -n hw.logicalcpu)` — must compile cleanly with no errors
2. Run the binary: `./build/physics_sim` (or whatever the output binary is named)
3. Visually verify:
   - Balls fall and settle under gravity
   - No balls escape the container
   - Changing restitution with +/- changes bounce behavior
   - No visible tunneling or explosion at rest
4. Run for at least 60 seconds without crashing

## Commit Policy
- Commit after each of these milestones (use descriptive messages):
  1. Blank SDL3 window opens
  2. Walls render correctly
  3. Single ball falls and bounces off floor
  4. Ball-ball collisions work
  5. 1000-ball scene is stable
  6. Performance optimized (spatial hashing)
  7. HUD and controls working
  8. Final polish and README complete
- Use `git add -A && git commit -m "..."` for each milestone
- Do NOT commit if the code does not compile

## Documentation
- Keep `README.md` updated with: how to build, how to run, controls, and any known issues
- Keep `PROGRESS.md` updated each session with: what was done, what was tested, what remains

## Known Hard Problems — Pay Special Attention
- **Penetration resolution**: always push overlapping balls apart *positionally* after computing velocity response. Without this, balls clip through each other.
- **Wall tunneling**: for fast-moving balls, check if ball crossed the wall in the last frame (swept collision). Use a small timestep or CCD if needed.
- **Vibration/energy explosion**: add a restitution bias — if relative normal velocity is below a small threshold (e.g., 0.5 units/s), treat restitution as 0 for that contact. This kills jitter without affecting real bounces.
- **Stacking stability**: use multiple solver iterations (8–16) and positional correction with a slop value (allow tiny overlap before correcting) to keep stacks stable.

## When Stuck
- If SDL3 API is unfamiliar, check: `brew info sdl3` and https://wiki.libsdl.org/SDL3
- If physics is unstable, reduce timestep or increase solver iterations before trying anything else
- If performance is bad, implement spatial hashing first before any other optimization
- Always prefer correctness over performance — get it stable, then get it fast

## Current Issues (fix these first)
- Simulation is running too slow — balls move sluggishly, not at real-time speed
- Animation is not smooth — appears choppy/laggy
- Performance needs improvement for 1000 balls

## Performance Fixes Needed
- Verify fixed timestep is correct (should feel like real gravity, balls should fall fast)
- Implement or verify spatial hash grid for broad-phase collision detection
- Target 60 fps — check that the render loop is not bottlenecked
- Make sure delta time scaling is correct so physics speed matches real time