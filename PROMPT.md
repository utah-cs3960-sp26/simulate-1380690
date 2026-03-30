2D Physics Simulator
Goal
Build a 2D physics simulator in C++ using SDL3. ~1000 balls fall into a funnel/container, bounce around, and settle naturally. The final result should look satisfying and pleasant to watch.
Requirements

SDL3 + CMake on macOS (brew install sdl3 cmake)
~1000 circular balls with gravity, ball-wall and ball-ball collisions
Inelastic collisions with configurable restitution (default 0.5)
Balls must eventually come to a COMPLETE stop — no jitter, no vibration, no infinite motion
60 fps target, 30 fps minimum

Scene Layout
Create a visually simple container using angled walls.

Two walls that funnel the walls into the box. 
The box should be wide enough to hold all 1000 balls when settled
Balls spawn above the funnel mouth in a grid, fall through the funnel, and pile up in the box

Physics — implement exactly this way

Fixed timestep: 1/120s. Run multiple substeps per frame to catch up if needed.
Each substep:
a. Apply gravity: vy += 980 * dt (use pixels, 980 px/s² feels realistic)
b. Integrate position: x += vx*dt, y += vy*dt
c. Run 10 iterations of collision resolution:

Ball vs wall segments: project ball onto wall, if penetrating push out along normal and reflect velocity component along normal multiplied by restitution. If relative normal velocity < 1.0, use restitution=0 for that contact.
Ball vs ball: if overlapping, push both apart equally, apply impulse with restitution. If relative normal velocity < 1.0, use restitution=0 for that contact.


After all substeps: if ball speed < 5.0 px/s, set velocity to exactly zero (sleep). Wake ball if hit by another.
Apply damping each substep: multiply velocity by 0.995

Restitution

Global value, default 0.5, range [0.0, 1.0]
+ key increases by 0.05, - decreases by 0.05
Lower restitution = faster settling, same final pile size either way

Controls

Q/Escape: quit
R: reset scene
+/-: change restitution
Space: pause/unpause
S: save ball positions to final_positions.csv

Rendering

Black background
White or gray wall segments, drawn thick (3px)
Balls drawn as filled circles, default color white or random pastel
HUD showing FPS, ball count, restitution value
Window: 1280x720

Week 11: CSV + Image Reveal
After physics is stable, add:
CSV scene loading

./physics_sim --scene file.csv loads ball starting positions and colors
CSV format: x,y,r,color_r,color_g,color_b (one ball per row)
S key saves current positions as final_positions.csv in same format
Without --scene, use default random scene

Color assignment tool: tools/assign_colors.py
Usage: python tools/assign_colors.py initial.csv final_positions.csv image.png output.csv

For each ball, find where it ended up in final_positions.csv
Sample the image color at that pixel location (scale image to window size)
Write output.csv with same starting positions but image-sampled colors
Requires Pillow: pip install pillow

Full workflow

Run sim with default scene → press S → get final_positions.csv
Run assign_colors.py with your image → get output.csv
Run sim with output.csv → watch image appear as balls settle

Commit Policy

git add -A && git commit -m "message" after EVERY successful compile
One change per commit, descriptive messages
Never commit broken code

KEY ISSUES: MUST BE FIXED
1) The balls vibrate after they finish falling. Make the restitution amount configurable; you should see things settle down faster with less restitution, but the final "settled" state should take up the same amount of space no matter the amount of restitution.
