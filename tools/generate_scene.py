#!/usr/bin/env python3
"""
Generate an initial scene CSV with ~1000 balls placed in a grid above the funnel.

Usage:
    python tools/generate_scene.py > initial.csv
    python tools/generate_scene.py -o initial.csv
"""

import random
import sys


def main():
    output_file = None
    for i, arg in enumerate(sys.argv[1:], 1):
        if arg == "-o" and i < len(sys.argv) - 1:
            output_file = sys.argv[i + 1]
            break

    random.seed(42)

    target = 1000
    spacing = 13.0
    spawn_left = 120.0
    spawn_right = 1280.0 - 120.0  # window width minus margin
    spawn_top = -800.0

    cols = int((spawn_right - spawn_left) / spacing)
    lines = []
    count = 0

    row = 0
    while count < target:
        for col in range(cols):
            if count >= target:
                break
            r = round(random.uniform(4.0, 6.0), 5)
            x = round(spawn_left + col * spacing + random.uniform(-0.2, 0.2), 3)
            y = round(spawn_top + row * spacing + random.uniform(-0.2, 0.2), 3)
            cr = random.randint(150, 255)
            cg = random.randint(150, 255)
            cb = random.randint(150, 255)
            lines.append(f"{x},{y},{r},{cr},{cg},{cb}")
            count += 1
        row += 1

    text = "\n".join(lines) + "\n"

    if output_file:
        with open(output_file, "w") as f:
            f.write(text)
        print(f"Wrote {count} balls to {output_file}", file=sys.stderr)
    else:
        sys.stdout.write(text)
        print(f"Generated {count} balls", file=sys.stderr)


if __name__ == "__main__":
    main()
