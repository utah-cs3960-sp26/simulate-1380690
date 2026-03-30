# Progress Log

## Session 1 — Initial Implementation

### Completed
- [x] SDL3 window opens (1280×720)
- [x] Wall rendering (thick gray lines via triangle geometry)
- [x] Ball rendering (scanline filled circles)
- [x] Gravity applied per substep
- [x] Ball-wall collision detection and response (closest point on segment)
- [x] Ball-ball collision detection and response (momentum exchange)
- [x] Positional correction with slop for ball-ball
- [x] Restitution bias for low-velocity contacts
- [x] 1000-ball scene spawned in grid above funnel
- [x] Spatial hash grid for broad-phase collision detection
- [x] Fixed timestep (1/120s) with accumulator
- [x] 10 solver iterations per substep
- [x] Velocity damping for near-rest balls
- [x] HUD: FPS, ball count, restitution, pause indicator
- [x] Controls: Q/Esc quit, R reset, Space pause, +/- restitution
- [x] Container with 4 walls + interior funnel
- [x] Wall normals auto-computed to point toward screen center
- [x] README.md with build instructions and design notes

### Tested
- Compiles cleanly with no errors
- Runs for 30+ seconds without crashing
- Balls fall under gravity and settle
- No visible tunneling or energy explosion at rest
- Restitution changes affect bounce behavior

### Known Issues
- None observed so far

## Session 2 — Performance & Smoothness Fixes

### Problems Fixed
- **Sluggish simulation speed**: Fixed timestep accumulator was consuming fractional substeps incorrectly (using `std::min(accum, fixed_dt)` which ran partial-sized steps). Now uses proper accumulator pattern — only runs full-size fixed_dt steps.
- **Choppy animation**: VSync was already enabled but physics was doing too much work per frame. Reduced solver iterations from 10 to 4 for better frame budget.
- **Spatial hash performance**: Replaced `std::vector<std::vector<int>>` (heap allocs per cell per rebuild) with counting-sort-based flat arrays (`grid_counts_`, `grid_starts_`, `grid_data_`). Eliminates thousands of heap allocations per frame.
- **Rendering bottleneck**: Replaced per-circle, per-scanline `SDL_RenderFillRect` calls (~13,000 individual draw calls) with a single batched `SDL_RenderFillRects` call using a pre-built rect array.

### Remaining
- Extended stress testing (60+ seconds)
- Visual verification of edge cases (high restitution, rapid resets)

## Session 3 — Sleep System, CSV Support, Image Reveal

### Completed
- [x] Fixed header declarations (apply_gravity, build_spatial_hash) to match implementation
- [x] Sleep system: balls track consecutive low-speed frames, sleep after 30 frames below 5 px/s threshold
- [x] Sleeping balls skip gravity and velocity integration; wake on significant collision
- [x] Restitution cutoff: contacts with relative normal velocity < 1.0 units/s use e=0.0
- [x] Velocity damping factor 0.999 applied per substep for subtle energy bleed
- [x] Positional correction slop increased to 0.5px to reduce oscillation
- [x] Solver iterations increased to 8 for better stacking stability
- [x] Per-ball RGB colors (each ball stores color_r, color_g, color_b)
- [x] Renderer draws each ball in its own color
- [x] CSV scene loading: `--scene initial.csv` command line argument
- [x] CSV scene saving: press S to save `final_positions.csv`
- [x] CSV format: x, y, radius, color_r, color_g, color_b
- [x] Color assignment tool: `tools/assign_colors.py` samples target image at final positions
- [x] README updated with image reveal workflow, CSV format, new controls
- [x] Removed unused prev_pos field from Ball struct

### Tested
- Compiles cleanly with no errors
- All features build and link correctly

### Remaining
- End-to-end test of image reveal workflow
- Extended runtime stress testing
- Visual verification of sleep system behavior

## Session 6 — Critical Bug Fixes

### Problems Fixed
- **Balls passing through walls**: Replaced discrete-only overlap check with swept collision detection using `prev_pos`. Now detects wall crossings even when balls move fast enough to skip past in one substep. Full positional correction (no partial slop) for wall collisions.
- **Not all balls dropping**: Balls were being spawned below the funnel (grid filled entire container). Fixed spawn to only place balls ABOVE the funnel walls. Lowered funnel to 55% height and extended funnel arms to create more spawn area, fitting all 1000 balls.
- **Balls don't fall naturally**: Reduced damping from 0.998 to 0.9995 (much less drag). Reduced wall friction from 0.3 to 0.03. Removed progressive damping before sleep. Made sleep contact-based (must be touching something AND moving slowly for 120 frames). Removed over-aggressive sleep waking.
- **Wall normal robustness**: Wall collision now derives face normal from segment geometry and orients it based on `prev_pos` side, rather than relying solely on precomputed normals.

### Tuning Changes
- Funnel position: 35% → 55% of window height
- Funnel arm length: 80px → 100px
- Ball spacing: 14px → 13px
- Sleep threshold: 8.0 → 3.0 px/s
- Sleep frames: 60 → 120
- Damping: 0.998 → 0.9995
- Wall friction: 0.3 → 0.03
- Ball-ball positional correction: 0.4 → 0.6

### Tested
- Compiles cleanly with no errors
- 1000 balls spawned correctly above funnel
- Runs for 3+ seconds without crashing

## Session 4 — Stability & Performance Fixes

### Problems Fixed
- **Spatial hash rebuilt every solver iteration**: Now only rebuilt once per substep (first iteration), reducing rebuild cost by 8x
- **Sleep system wake bugs**: Fixed ball-wall wake logic — only wakes sleeping balls on significant incoming velocity (>2.0 units/s), not on micro-penetrations from gravity settling
- **Ball-ball sleep instability**: Skip positional correction for both-sleeping pairs (stable stacks), don't apply impulse to sleeping balls, use wake threshold for ball-ball collisions
- **Wall collision restitution**: Applied restitution cutoff consistently in degenerate (dist < epsilon) wall collision branch
- **Accumulator spiral of death**: Capped accumulator to max 8 substeps per frame to prevent lag cascade
- **Auto-save initial.csv**: Default scene now auto-saves initial.csv on startup for image reveal workflow

### Added
- Sleeping ball count displayed in HUD

### Tested
- Compiles cleanly with no errors
- Runs for 5+ seconds without crashing
- initial.csv auto-generated on default scene startup

## Session 5 — Stability & Performance Improvements

### Problems Fixed
- **Vibration at rest**: Improved sleep system with progressive damping as balls approach sleep threshold, higher wake thresholds (5.0 for walls, 5.0 for ball-ball) to prevent gravity micro-collisions from waking settled balls
- **Wall collision jitter**: Added positional correction slop (0.3px) to wall collisions, sleeping balls only have penetrating velocity zeroed without full reflection
- **Ball-ball over-correction**: Reduced positional correction factor to 0.4 to prevent oscillation, both-sleeping pairs get only gentle correction for deep overlaps
- **Duplicate pair resolution**: Refactored solver to collect unique collision pairs via sort+unique before iterating, preventing same pair from being resolved multiple times per iteration
- **Rendering performance**: Batch circle rects by color with pre-reserved vector capacity
- **Friction**: Added tangential friction (0.3) to wall collisions to help balls settle laterally

### Tuning Changes
- Sleep speed threshold: 5.0 → 8.0 px/s
- Sleep frames required: 30 → 60
- Velocity damping: 0.999 → 0.998
- Restitution cutoff velocity: 1.0 → 2.0 units/s
- Solver iterations: 8 → 10
- Max substeps: 4 → 8
- Ball radius range: 4-7 → 4-6 for better packing
- Pillow installed for assign_colors.py

### Tested
- Compiles cleanly with no errors
- Runs for 15+ seconds without crashing
- 1000 balls generated, initial.csv auto-saved correctly
