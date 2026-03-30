Physics Simulator - Amp Autonomous Build Prompt
Project Goal
Build a 2D physics simulator using C++ and SDL3 that simulates ~1000 circular balls inside a container made of fixed walls. Balls are influenced by gravity and collide with walls and each other using inelastic collisions with configurable restitution. The simulation must be stable and visually smooth — balls should fall, bounce, and eventually come to rest naturally.
Tech Stack

Language: C++17 or later
Graphics/Window: SDL3 (brew install sdl3 if needed)
Build system: CMake (brew install cmake if needed)
Platform: macOS

What "Working" Looks Like

~1000 balls spawn above the floor inside a container
Balls fall under gravity and bounce off walls and each other
Balls gradually lose energy and come to a complete stop at the bottom
No balls escape the container, tunnel through walls, or spin in endless loops
Runs at 30+ fps
Pressing +/- changes restitution and visibly changes how bouncy the balls are

Current State — THE SIMULATION IS BROKEN
The current implementation has balls forming a vortex/tunnel in the middle that never stops. This is fundamentally wrong. Start over with a clean, simple implementation rather than trying to patch the existing broken code. Delete src/ and start fresh if needed.
Simple, Correct Implementation Approach
Do NOT over-engineer this. Use this straightforward approach:
Physics Loop (per frame)

Apply gravity to all ball velocities: vy += gravity * dt
Move all balls: x += vx * dt, y += vy * dt
Resolve ball-wall collisions (push ball out, reflect velocity * restitution)
Resolve ball-ball collisions (push apart, exchange momentum * restitution)
Repeat steps 3-4 for 10 iterations to handle stacking
If a ball's speed < 2.0, set its velocity to zero (sleep it)

Restitution

Single global value, default 0.5
Applied to velocity after collision: v_new = -v * restitution
Changeable at runtime with +/- keys

Container

Simple rectangle: left wall, right wall, floor, ceiling
Hardcode the wall positions — no need for a complex wall system

Ball Setup

Place ~1000 balls in a grid pattern near the top of the container
Radius: 5px, slight variation okay
No initial velocity

Stability Rules

After collision response, always push balls apart so they don't overlap
If relative collision velocity < 1.0, use restitution = 0 for that contact
Apply 0.99 velocity damping per frame

Performance

Use a spatial hash grid for ball-ball broad phase (required for 1000 balls at 30fps)
Fixed timestep of 1/120s, multiple substeps per render frame if needed

Files

CMakeLists.txt
src/main.cpp — SDL3 init, main loop, input handling
src/simulation.h/.cpp — physics
src/renderer.h/.cpp — drawing
README.md — how to build and run
PROGRESS.md — what was done each session

Controls

Q / Escape: quit
R: reset scene
+ / -: restitution up/down by 0.05
Space: pause/unpause
S: save ball positions to final_positions.csv

Commit Policy

Commit after EVERY successful compile with git add -A && git commit -m "description"
Small frequent commits — do not batch changes
Never commit broken/non-compiling code

Testing After Every Change
cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(sysctl -n hw.logicalcpu)
./physics_sim
Verify: balls fall, bounce, and settle. No vortex. No infinite motion.

Week 9: Image Reveal Feature (implement AFTER physics is stable)
Goal
Balls bounce around randomly, then settle to reveal an image. Each ball gets colored based on where it ends up in the final pile.
Workflow

User runs simulator with default colors → presses S → saves final_positions.csv
User runs: python tools/assign_colors.py scene.csv final_positions.csv image.png colored.csv
User runs simulator with ./physics_sim --scene colored.csv → image appears when balls settle

CSV Format
One row per ball: x, y, r, color_r, color_g, color_b
assign_colors.py

Input: initial scene CSV, final positions CSV, target image (PNG/JPG)
For each ball: look up its final position, sample image color at that pixel, assign to ball
Output: new CSV with same starting positions but image-sampled colors
Use Python + Pillow (pip install pillow)
Scale image to simulator window size if dimensions differ

Simulator Changes

Accept --scene filename.csv command line arg to load a scene
Fall back to default random scene if no arg given
S key saves current positions to final_positions.csv
