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

### Remaining
- Extended stress testing (60+ seconds)
- Visual verification of edge cases (high restitution, rapid resets)
