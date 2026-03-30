# Results

## Week 8

### Physics Simulator

A 2D physics simulator built with C++17 and SDL3. ~1000 balls with varying radii (4–6 px) fall through a V-shaped funnel into a rectangular container and settle under gravity with configurable restitution.

**Key physics features:**
- Fixed timestep (1/120s) with up to 8 substeps per frame
- Gravity at 980 px/s², velocity damping at 0.995/substep
- 10 solver iterations per substep for stable stacking
- Spatial hash grid for O(n) broad-phase collision detection
- Sleep system: balls sleep when speed < 5.0 px/s for 20 consecutive substeps
- Low-velocity restitution cutoff: contacts with relative velocity < 15 px/s use e=0

**Controls:** Q/Esc quit, R reset, Space pause, +/- restitution (0.0–1.0), S save CSV.

**Build and run:**
```bash
mkdir -p build && cd build && cmake .. && make -j$(sysctl -n hw.logicalcpu)
./build/physics_sim
```

## Week 11

### Image Reveal Workflow

The simulator supports an end-to-end workflow where balls are colored to reveal a target image once they settle into their final positions.

**CSV format:** `x,y,radius,color_r,color_g,color_b` (one ball per row).

### End-to-End Steps

**Step 1: Generate the initial scene**
```bash
python3 tools/generate_scene.py -o initial.csv
```
Creates `initial.csv` with ~1000 balls in a grid above the funnel, all with random pastel colors.

**Step 2: Run the simulator and capture final positions**
```bash
./build/physics_sim
# Wait for balls to settle, then press S to save final_positions.csv
# Press Q to quit
```

**Step 3: Assign image colors to balls**
```bash
python3 tools/assign_colors.py initial.csv final_positions.csv Manchester-United-Logo-1.png output.csv
```
For each ball, samples the pixel color from the target image at the ball's final resting position (scaled to 1280×720), then writes `output.csv` with the original starting positions but the image-sampled colors.

**Step 4: Run the simulator with colored balls**
```bash
./build/physics_sim --scene output.csv
```
The balls fall through the funnel and settle into the same positions — revealing the target image as they come to rest.

### Tools

| Tool | Purpose |
|------|---------|
| `tools/generate_scene.py` | Generate `initial.csv` with ~1000 balls in a grid above the funnel |
| `tools/assign_colors.py` | Sample colors from a PNG at final ball positions and write a new CSV |
