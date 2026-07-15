#!/usr/bin/env python3
"""
Reduce animation frame count by keeping every Nth frame.
Usage: python reduce_frames.py <prefix> <step>
Example: python reduce_frames.py cat_sleep 3  (keep every 3rd frame: 35→12)
"""
import sys, os, re

def reduce_frames(prefix, step, h_dir='Core/Inc', c_dir='Core/Src'):
    c_path = os.path.join(c_dir, f'{prefix}.c')
    h_path = os.path.join(h_dir, f'{prefix}.h')

    # Read .c file
    with open(c_path, 'r', encoding='utf-8') as f:
        c_text = f.read()

    # Parse frame arrays: find all "const uint8_t prefix_fN[1024] = { ... };"
    pattern = rf'const uint8_t {prefix}_f(\d+)\[1024\] = \{{([^}}]+)\}};'
    matches = list(re.finditer(pattern, c_text, re.DOTALL))
    if not matches:
        print(f"ERROR: No frames found in {c_path}")
        return

    print(f"Found {len(matches)} frames in {c_path}")

    # Select frames: keep every Nth frame (0-indexed: 0, step, 2*step, ...)
    selected = matches[::step]
    print(f"Keeping {len(selected)} frames (step={step})")

    # Renumber frames: f0, f1, f2, ...
    new_frames = []
    for new_idx, m in enumerate(selected):
        body = m.group(2)  # hex bytes content
        new_frames.append((new_idx, body))

    # Generate new .c
    lines = [f'#include "{prefix}.h"', '']
    for new_idx, body in new_frames:
        lines.append(f'const uint8_t {prefix}_f{new_idx}[1024] = {{')
        lines.append(f'    {body}}};')
        lines.append('')

    # Frame pointer array
    num = len(new_frames)
    refs = ', '.join(f'{prefix}_f{i}' for i in range(num))
    lines.append(f'const uint8_t *{prefix}_frames[{num}] = {{')
    lines.append(f'    {refs}')
    lines.append('};')
    lines.append('')

    with open(c_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    print(f"  Rewrote: {c_path} ({num} frames)")

    # Generate new .h
    guard = f'__{prefix.upper()}_H'
    h_lines = [
        f'#ifndef {guard}',
        f'#define {guard}',
        f'#include "main.h"',
        f'#define {prefix.upper()}_FRAMES  {num}',
        '',
    ]
    for i in range(num):
        h_lines.append(f'extern const uint8_t {prefix}_f{i}[1024];')
    h_lines.append(f'extern const uint8_t *{prefix}_frames[{num}];')
    h_lines.append('')
    h_lines.append(f'#endif /* {guard} */')
    h_lines.append('')

    with open(h_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(h_lines))
    print(f"  Rewrote: {h_path} ({num} frames)")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    reduce_frames(sys.argv[1], int(sys.argv[2]))
