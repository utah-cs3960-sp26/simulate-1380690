Physics Simulator - Amp Autonomous Build Prompt
Project Goal
Build a 2D physics simulator using C++ and SDL3 that simulates ~1000 circular balls inside a container made of fixed walls. The simulation should feature gravity, ball-ball collisions, and ball-wall collisions with configurable restitution (inelastic collisions). The simulation must be stable (no tunneling, no infinite vibration/energy explosions) and visually smooth.
Tech Stack

Language: C++17 or later
Graphics/Window: SDL3 (install via Homebrew if not present: brew install sdl3)
Build system: CMake
Platform: macOS

Setup Steps (do these first)

Check if SDL3 is installed: brew list sdl3 2>/dev/null || brew install sdl3
Check if CMake is installed: brew list cmake 2>/dev/null || brew install cmake
Initialize a CMakeLists.txt that links against SDL3
Verify the project compiles and a blank SDL3 window opens before adding physics

Architecture
Files to create

CMakeLists.txt — build config
src/main.cpp — entry point, SDL3 init, main loop
src/simulation.h / src/simulation.cpp — physics world: balls, walls, step()
src/renderer.h / src/renderer.cpp — SDL3 rendering of balls and walls
README.md — build instructions, controls, design notes
PROGRESS.md — running log of what has been implemented and what still needs fixing

Core Data Structures

Ball: position (x,y), velocity (vx,vy), radius, mass (proportional to radius^2)
Wall: two endpoints (line segment), normal vector — fixed and immovable
World: list of balls, list of walls, gravity constant, restitution coefficient

Physics Requirements
Gravity

Apply downward acceleration each frame (e.g., 9.8 units/s²), scaled by delta time

Collision Detection & Response

Ball-wall: signed distance from ball center to wall line; if < radius, resolve penetration and reflect velocity using restitution
Ball-ball: if distance between centers < sum of radii, resolve overlap and exchange momentum using restitution
Restitution coefficient e (0.0 = perfectly inelastic, 1.0 = perfectly elastic) should be a runtime-configurable constant (e.g., defined at top of simulation.cpp or passed via constructor). Default: 0.5
Penetration correction: after collision response, push balls apart positionally so they do not overlap — this is critical to prevent tunneling and vibration explosions
Use iterative collision resolution: run several solver iterations per frame (e.g., 8–16 iterations) to handle stacking and clusters

Stability Requirements (very important)

Balls must NEVER tunnel through walls or other balls
Balls must NOT gain energy over time — damp residual velocity when speed is very low (sleep threshold)
With restitution = 0.0, balls should come to rest quickly; with restitution = 0.9, they should bounce a long time but still eventually settle
Final resting volume/area of the pile should be approximately the same regardless of restitution (same number of balls, same sizes)

Scene Setup

Container: a rectangular "box" made of 4 wall segments, plus optionally a funnel or ramp in the middle to make it visually interesting
~1000 balls of slightly varying radii (e.g., 4–8 px), placed in a grid or random scatter above the container floor with no initial overlap
Initial velocity: zero or small random jitter

Rendering

Render at 60 fps target using SDL3
Draw walls as thick white or gray lines
Draw balls as filled circles (use a simple midpoint circle or SDL_RenderFillCircle approximation)
Show a simple HUD (SDL_RenderDebugText or a basic overlay) displaying: FPS, ball count, restitution value
Window size: 1280x720 or 1024x768

Controls

Q or Escape: quit
R: reset scene
+ / -: increase/decrease restitution by 0.05 (clamped to [0.0, 1.0])
Space: pause/unpause

Performance

The simulation must run at interactive frame rates (>= 30 fps) with 1000 balls on a modern Mac
Use a spatial hash grid or similar broad-phase collision detection to avoid O(n²) ball-ball checks
Keep the physics timestep fixed (e.g., 1/120s) and allow multiple substeps per render frame if needed

Testing Protocol (run after every major change)

mkdir -p build && cd build && cmake .. && make -j$(sysctl -n hw.logicalcpu) — must compile cleanly with no errors
Run the binary: ./build/physics_sim (or whatever the output binary is named)
Visually verify:

Balls fall and settle under gravity
No balls escape the container
Changing restitution with +/- changes bounce behavior
No visible tunneling or explosion at rest


Run for at least 60 seconds without crashing

Commit Policy — VERY IMPORTANT

Commit after every individual change, not just at major milestones. If you edit a file, compile, and it works — commit it immediately.
Do not batch up many changes into one commit. Small, frequent commits are required.
Use descriptive commit messages that describe exactly what changed (e.g., "Fix ball-wall penetration resolution", "Add spatial hash grid", "Reduce vibration with sleep threshold")
After every successful make, run git add -A && git commit -m "..." before moving on to the next change
Do NOT commit if the code does not compile
Run git log --oneline at the end of each session to verify commits were made throughout

Documentation

Keep README.md updated with: how to build, how to run, controls, and any known issues
Keep PROGRESS.md updated each session with: what was done, what was tested, what remains

Known Hard Problems — Pay Special Attention
Vibration / Jitter (TOP PRIORITY — currently broken)
Balls are currently vibrating at rest instead of settling cleanly. This must be fixed:

Sleep system: each ball must track how long it has been moving slowly. If a ball's speed stays below a threshold (e.g., 5 units/s) for several consecutive frames, mark it as "sleeping" and skip applying gravity and velocity updates to it. Wake it up if another ball hits it.
Restitution cutoff: if the relative normal velocity between two colliding objects is below a small threshold (e.g., 1.0 units/s), set restitution to 0.0 for that contact only. This prevents micro-bounces that cause jitter.
Velocity damping: apply a small linear damping factor to all ball velocities each frame (e.g., multiply velocity by 0.99) to bleed off residual energy gradually.
Positional correction slop: when correcting penetration, allow a tiny slop (e.g., 0.5px) before applying correction, so the solver doesn't over-correct and create oscillation.
Do NOT use large damping as a shortcut — it makes fast bounces feel wrong. Use the sleep system for resting balls and keep damping subtle.

Penetration Resolution

Always push overlapping balls apart positionally after computing velocity response. Without this, balls clip through each other.

Wall Tunneling

For fast-moving balls, check if ball crossed the wall in the last frame (swept collision). Use a small timestep or CCD if needed.

Stacking Stability

Use multiple solver iterations (8–16) and positional correction with a slop value to keep stacks stable.

When Stuck

If SDL3 API is unfamiliar, check: brew info sdl3 and https://wiki.libsdl.org/SDL3
If physics is unstable, reduce timestep or increase solver iterations before trying anything else
If performance is bad, implement spatial hashing first before any other optimization
Always prefer correctness over performance — get it stable, then get it fast

Current Issues (fix these first)

Balls vibrate at rest — balls that should be settled keep jittering and wiggling. Implement the sleep system and restitution cutoff described above.
Simulation runs too slow — balls move sluggishly, not at real-time speed. Fix delta time scaling.
The balls MUST come to a rest at the end.
Animation is choppy — target 60 fps with spatial hash grid for collision detection.

Performance Fixes Needed

Verify fixed timestep is correct (should feel like real gravity, balls should fall fast)
Implement or verify spatial hash grid for broad-phase collision detection
Target 60 fps — check that the render loop is not bottlenecked
Make sure delta time scaling is correct so physics speed matches real time

Week 9 Feature: Image Reveal via CSV Scene Description
Goal
Make the simulator able to load an initial scene from a CSV file and save final positions to a CSV file. Then add a separate tool that pre-assigns ball colors based on what color a target image has at each ball's final resting position. The result: balls start scattered and colorful (seemingly random), bounce around, and when they settle the image becomes visible formed by the colored balls.
See examples of this effect on YouTube: search "ball sort image reveal physics simulation".
CSV Input Format

One row per ball
Columns: x, y, r, color_r, color_g, color_b

x, y: starting position in pixels
r: radius (optional, can be fixed if not present)
color_r, color_g, color_b: RGB color values 0–255


Walls can remain hardcoded or optionally be included in a separate section/file
Example row: 512, 100, 5, 255, 0, 128

CSV Output Format

Same format as input, but with final resting positions after simulation settles
Save automatically when user presses S or when simulation is paused and settled
Output filename: final_positions.csv

Color Assignment Tool
Create a separate standalone tool (e.g., tools/assign_colors.py or tools/assign_colors.cpp) that:

Takes as input:

An initial scene CSV (with starting positions)
A target image file (PNG or JPG)
A final positions CSV (output from the simulator)


For each ball, looks up where it ended up in the final positions CSV, samples the target image color at that (x, y) pixel location, and assigns that color to the ball
Outputs a new initial scene CSV with the same starting positions but colors assigned based on final destination
The user workflow is:

Run simulator once with any colors → get final_positions.csv
Run the color tool: python tools/assign_colors.py initial.csv final_positions.csv image.png output.csv
Run simulator again with output.csv → balls reveal the image when settled



Implementation Notes

The color tool can be Python (preferred for simplicity) using Pillow for image loading
Scale image coordinates to match simulator window size if they differ
If a ball's final position falls outside the image bounds, assign white or gray
The simulator should accept a CSV filename as a command line argument: ./physics_sim --scene initial.csv
If no CSV is provided, fall back to the default random scene

Controls Addition

S: save current ball positions to final_positions.csv

Commit Policy (same as before — commit after every change)

Commit after adding CSV load support
Commit after adding CSV save support
Commit after building the color assignment tool
Commit after testing the full end-to-end workflowPhysics Simulator - Amp Autonomous Build Prompt
Project Goal
Build a 2D physics simulator using C++ and SDL3 that simulates ~1000 circular balls inside a container made of fixed walls. The simulation should feature gravity, ball-ball collisions, and ball-wall collisions with configurable restitution (inelastic collisions). The simulation must be stable (no tunneling, no infinite vibration/energy explosions) and visually smooth.
Tech Stack

Language: C++17 or later
Graphics/Window: SDL3 (install via Homebrew if not present: brew install sdl3)
Build system: CMake
Platform: macOS

Setup Steps (do these first)

Check if SDL3 is installed: brew list sdl3 2>/dev/null || brew install sdl3
Check if CMake is installed: brew list cmake 2>/dev/null || brew install cmake
Initialize a CMakeLists.txt that links against SDL3
Verify the project compiles and a blank SDL3 window opens before adding physics

Architecture
Files to create

CMakeLists.txt — build config
src/main.cpp — entry point, SDL3 init, main loop
src/simulation.h / src/simulation.cpp — physics world: balls, walls, step()
src/renderer.h / src/renderer.cpp — SDL3 rendering of balls and walls
README.md — build instructions, controls, design notes
PROGRESS.md — running log of what has been implemented and what still needs fixing

Core Data Structures

Ball: position (x,y), velocity (vx,vy), radius, mass (proportional to radius^2)
Wall: two endpoints (line segment), normal vector — fixed and immovable
World: list of balls, list of walls, gravity constant, restitution coefficient

Physics Requirements
Gravity

Apply downward acceleration each frame (e.g., 9.8 units/s²), scaled by delta time

Collision Detection & Response

Ball-wall: signed distance from ball center to wall line; if < radius, resolve penetration and reflect velocity using restitution
Ball-ball: if distance between centers < sum of radii, resolve overlap and exchange momentum using restitution
Restitution coefficient e (0.0 = perfectly inelastic, 1.0 = perfectly elastic) should be a runtime-configurable constant (e.g., defined at top of simulation.cpp or passed via constructor). Default: 0.5
Penetration correction: after collision response, push balls apart positionally so they do not overlap — this is critical to prevent tunneling and vibration explosions
Use iterative collision resolution: run several solver iterations per frame (e.g., 8–16 iterations) to handle stacking and clusters

Stability Requirements (very important)

Balls must NEVER tunnel through walls or other balls
Balls must NOT gain energy over time — damp residual velocity when speed is very low (sleep threshold)
With restitution = 0.0, balls should come to rest quickly; with restitution = 0.9, they should bounce a long time but still eventually settle
Final resting volume/area of the pile should be approximately the same regardless of restitution (same number of balls, same sizes)

Scene Setup

Container: a rectangular "box" made of 4 wall segments, plus optionally a funnel or ramp in the middle to make it visually interesting
~1000 balls of slightly varying radii (e.g., 4–8 px), placed in a grid or random scatter above the container floor with no initial overlap
Initial velocity: zero or small random jitter

Rendering

Render at 60 fps target using SDL3
Draw walls as thick white or gray lines
Draw balls as filled circles (use a simple midpoint circle or SDL_RenderFillCircle approximation)
Show a simple HUD (SDL_RenderDebugText or a basic overlay) displaying: FPS, ball count, restitution value
Window size: 1280x720 or 1024x768

Controls

Q or Escape: quit
R: reset scene
+ / -: increase/decrease restitution by 0.05 (clamped to [0.0, 1.0])
Space: pause/unpause

Performance

The simulation must run at interactive frame rates (>= 30 fps) with 1000 balls on a modern Mac
Use a spatial hash grid or similar broad-phase collision detection to avoid O(n²) ball-ball checks
Keep the physics timestep fixed (e.g., 1/120s) and allow multiple substeps per render frame if needed

Testing Protocol (run after every major change)

mkdir -p build && cd build && cmake .. && make -j$(sysctl -n hw.logicalcpu) — must compile cleanly with no errors
Run the binary: ./build/physics_sim (or whatever the output binary is named)
Visually verify:

Balls fall and settle under gravity
No balls escape the container
Changing restitution with +/- changes bounce behavior
No visible tunneling or explosion at rest


Run for at least 60 seconds without crashing

Commit Policy — VERY IMPORTANT

Commit after every individual change, not just at major milestones. If you edit a file, compile, and it works — commit it immediately.
Do not batch up many changes into one commit. Small, frequent commits are required.
Use descriptive commit messages that describe exactly what changed (e.g., "Fix ball-wall penetration resolution", "Add spatial hash grid", "Reduce vibration with sleep threshold")
After every successful make, run git add -A && git commit -m "..." before moving on to the next change
Do NOT commit if the code does not compile
Run git log --oneline at the end of each session to verify commits were made throughout

Documentation

Keep README.md updated with: how to build, how to run, controls, and any known issues
Keep PROGRESS.md updated each session with: what was done, what was tested, what remains

Known Hard Problems — Pay Special Attention
Vibration / Jitter (TOP PRIORITY — currently broken)
Balls are currently vibrating at rest instead of settling cleanly. This must be fixed:

Sleep system: each ball must track how long it has been moving slowly. If a ball's speed stays below a threshold (e.g., 5 units/s) for several consecutive frames, mark it as "sleeping" and skip applying gravity and velocity updates to it. Wake it up if another ball hits it.
Restitution cutoff: if the relative normal velocity between two colliding objects is below a small threshold (e.g., 1.0 units/s), set restitution to 0.0 for that contact only. This prevents micro-bounces that cause jitter.
Velocity damping: apply a small linear damping factor to all ball velocities each frame (e.g., multiply velocity by 0.99) to bleed off residual energy gradually.
Positional correction slop: when correcting penetration, allow a tiny slop (e.g., 0.5px) before applying correction, so the solver doesn't over-correct and create oscillation.
Do NOT use large damping as a shortcut — it makes fast bounces feel wrong. Use the sleep system for resting balls and keep damping subtle.

Penetration Resolution

Always push overlapping balls apart positionally after computing velocity response. Without this, balls clip through each other.

Wall Tunneling

For fast-moving balls, check if ball crossed the wall in the last frame (swept collision). Use a small timestep or CCD if needed.

Stacking Stability

Use multiple solver iterations (8–16) and positional correction with a slop value to keep stacks stable.

When Stuck

If SDL3 API is unfamiliar, check: brew info sdl3 and https://wiki.libsdl.org/SDL3
If physics is unstable, reduce timestep or increase solver iterations before trying anything else
If performance is bad, implement spatial hashing first before any other optimization
Always prefer correctness over performance — get it stable, then get it fast

Current Issues (fix these first)

1) Balls are moving through this walls, this is not allowed.
2) Not all the balls are dropping when the program is run, the balls must fall like a realistic simulator.
3) The balls don't fall naturally as they should.

Week 9 Feature: Image Reveal via CSV Scene Description
Goal
Make the simulator able to load an initial scene from a CSV file and save final positions to a CSV file. Then add a separate tool that pre-assigns ball colors based on what color a target image has at each ball's final resting position. The result: balls start scattered and colorful (seemingly random), bounce around, and when they settle the image becomes visible formed by the colored balls.
See examples of this effect on YouTube: search "ball sort image reveal physics simulation".
CSV Input Format

One row per ball
Columns: x, y, r, color_r, color_g, color_b

x, y: starting position in pixels
r: radius (optional, can be fixed if not present)
color_r, color_g, color_b: RGB color values 0–255


Walls can remain hardcoded or optionally be included in a separate section/file
Example row: 512, 100, 5, 255, 0, 128

CSV Output Format

Same format as input, but with final resting positions after simulation settles
Save automatically when user presses S or when simulation is paused and settled
Output filename: final_positions.csv

Color Assignment Tool
Create a separate standalone tool (e.g., tools/assign_colors.py or tools/assign_colors.cpp) that:

Takes as input:

An initial scene CSV (with starting positions)
A target image file (PNG or JPG)
A final positions CSV (output from the simulator)


For each ball, looks up where it ended up in the final positions CSV, samples the target image color at that (x, y) pixel location, and assigns that color to the ball
Outputs a new initial scene CSV with the same starting positions but colors assigned based on final destination
The user workflow is:

Run simulator once with any colors → get final_positions.csv
Run the color tool: python tools/assign_colors.py initial.csv final_positions.csv image.png output.csv
Run simulator again with output.csv → balls reveal the image when settled



Implementation Notes

The color tool can be Python (preferred for simplicity) using Pillow for image loading
Scale image coordinates to match simulator window size if they differ
If a ball's final position falls outside the image bounds, assign white or gray
The simulator should accept a CSV filename as a command line argument: ./physics_sim --scene initial.csv
If no CSV is provided, fall back to the default random scene

Controls Addition

S: save current ball positions to final_positions.csv

Commit Policy (same as before — commit after every change)

Commit after adding CSV load support
Commit after adding CSV save support
Commit after building the color assignment tool
Commit after testing the full end-to-end workflow
