#!/usr/bin/env python3
"""
Color assignment tool for the physics simulator image reveal effect.

Usage:
    python tools/assign_colors.py initial.csv final_positions.csv image.png output.csv

For each ball, looks up its final resting position, samples the target image
color at that pixel, and writes a new CSV with original starting positions
but colors from the image at the final position.
"""

import sys
import csv
from PIL import Image


def main():
    if len(sys.argv) != 5:
        print("Usage: python assign_colors.py <initial.csv> <final_positions.csv> <image.png> <output.csv>")
        sys.exit(1)

    initial_file = sys.argv[1]
    final_file = sys.argv[2]
    image_file = sys.argv[3]
    output_file = sys.argv[4]

    # Load initial positions
    initial_balls = []
    with open(initial_file, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = [p.strip() for p in line.split(",")]
            initial_balls.append(parts)

    # Load final positions
    final_balls = []
    with open(final_file, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = [p.strip() for p in line.split(",")]
            final_balls.append(parts)

    if len(initial_balls) != len(final_balls):
        print(f"Warning: initial ({len(initial_balls)}) and final ({len(final_balls)}) ball counts differ")
        # Use minimum
        count = min(len(initial_balls), len(final_balls))
        initial_balls = initial_balls[:count]
        final_balls = final_balls[:count]

    # Load target image
    img = Image.open(image_file).convert("RGB")
    img_w, img_h = img.size

    # Simulator window size (must match)
    SIM_W = 1280
    SIM_H = 720

    # Write output CSV
    with open(output_file, "w") as f:
        for i in range(len(initial_balls)):
            init = initial_balls[i]
            final = final_balls[i]

            # Starting position and radius from initial
            x_start = init[0]
            y_start = init[1]
            radius = init[2] if len(init) >= 3 else "5"

            # Final position for color sampling
            fx = float(final[0])
            fy = float(final[1])

            # Map simulator coords to image coords
            img_x = int(fx * img_w / SIM_W)
            img_y = int(fy * img_h / SIM_H)

            # Clamp to image bounds
            img_x = max(0, min(img_x, img_w - 1))
            img_y = max(0, min(img_y, img_h - 1))

            # Sample color
            r, g, b = img.getpixel((img_x, img_y))

            f.write(f"{x_start},{y_start},{radius},{r},{g},{b}\n")

    print(f"Wrote {len(initial_balls)} balls to {output_file}")


if __name__ == "__main__":
    main()
