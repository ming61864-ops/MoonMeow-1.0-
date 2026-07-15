#!/usr/bin/env python3
"""
全屏点阵转换脚本: 横向逐行格式 → SSD1306纵向逐页格式

输入: 取模txt文件 (const uint8_t bitmap_N[] = { 0x00, ... }; 格式, 每帧1024字节)
输出: .c + .h 文件, 含单个帧数组 + 帧指针数组

用法: python convert_fullscreen.py <输入txt> <输出变量前缀> [输出目录]
示例: python convert_fullscreen.py 月薪喵点阵1.txt cat_anim1
"""

import sys, os, re

def parse_frames(filepath):
    """从txt文件中提取所有bitmap_N帧数据, 返回 list[bytearray(1024)]"""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        text = f.read()

    # 找到所有 "bitmap_N[] = {" ... "};" 块
    # 有些文件有 "点阵数组数据:" 前缀
    pattern = r'(?:点阵数组数据:)?const\s+uint8_t\s+bitmap_(\d+)\[\]\s*=\s*\{([^}]+)\};'
    matches = re.findall(pattern, text, re.DOTALL)

    if not matches:
        print(f"ERROR: No bitmap data found in {filepath}")
        return []

    # 按编号排序
    matches.sort(key=lambda m: int(m[0]))

    frames = []
    for num_str, body in matches:
        # 解析 hex bytes (0x00 格式)
        hex_bytes = re.findall(r'0x([0-9A-Fa-f]{2})', body)
        if len(hex_bytes) != 1024:
            print(f"  WARNING: bitmap_{num_str} has {len(hex_bytes)} bytes (expected 1024)")
        data = bytearray(int(h, 16) for h in hex_bytes)
        # 补齐或截断到1024
        if len(data) < 1024:
            data.extend(b'\x00' * (1024 - len(data)))
        frames.append(data[:1024])

    print(f"  Parsed {len(frames)} frames from {filepath}")
    return frames

def transpose_h_to_v(data):
    """
    H→V转置: 横向逐行(MSB左) → SSD1306纵向逐页(LSB顶)

    输入: 1024字节, 按行排列 row[0..63], 每行16字节(128像素/8)
          每字节 bit7=最左像素, bit0=最右像素
    输出: 1024字节, 按页排列 page[0..7], 每页128列
          每字节 bit0=最上像素, bit7=最下像素
    """
    out = bytearray(1024)
    for page in range(8):           # 8 pages
        for col in range(128):       # 128 columns per page
            byte = 0
            for bit in range(8):     # 8 vertical pixels per byte
                row = page * 8 + bit
                col_byte = col // 8          # which input byte in this row
                col_bit  = 7 - (col % 8)     # bit position (MSB=leftmost)
                input_idx = row * 16 + col_byte
                if input_idx < 1024 and (data[input_idx] & (1 << col_bit)):
                    byte |= (1 << bit)
            out[page * 128 + col] = byte
    return out

def write_c_header(filepath, prefix, num_frames):
    """生成 .h 文件"""
    guard = f"__{prefix.upper()}_H"
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        f'#include "main.h"',
        f"#define {prefix.upper()}_FRAMES  {num_frames}",
        "",
    ]
    for i in range(num_frames):
        lines.append(f"extern const uint8_t {prefix}_f{i}[1024];")
    lines.append(f"extern const uint8_t *{prefix}_frames[{num_frames}];")
    lines.append("")
    lines.append(f"#endif /* {guard} */")
    lines.append("")

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    print(f"  Generated: {filepath}")

def write_c_source(filepath, prefix, frames_transposed):
    """生成 .c 文件"""
    lines = [
        f'#include "{prefix}.h"',
        "",
    ]

    for i, data in enumerate(frames_transposed):
        lines.append(f"const uint8_t {prefix}_f{i}[1024] = {{")
        # 每行16个hex值
        for row_start in range(0, 1024, 16):
            chunk = ', '.join(f'0x{b:02X}' for b in data[row_start:row_start+16])
            lines.append(f'    {chunk},')
        lines.append('};')
        lines.append('')

    # 帧指针数组
    num = len(frames_transposed)
    refs = ', '.join(f'{prefix}_f{i}' for i in range(num))
    lines.append(f"const uint8_t *{prefix}_frames[{num}] = {{")
    lines.append(f"    {refs}")
    lines.append('};')
    lines.append('')

    with open(filepath, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))
    print(f"  Generated: {filepath}")

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    input_txt  = sys.argv[1]
    prefix     = sys.argv[2]
    # 默认: .h → Core/Inc , .c → Core/Src
    h_dir = 'Core/Inc'
    c_dir = 'Core/Src'

    if not os.path.exists(input_txt):
        print(f"ERROR: File not found: {input_txt}")
        sys.exit(1)

    print(f"Converting: {input_txt} → {prefix}.c/.h")

    # Parse
    frames_raw = parse_frames(input_txt)
    if not frames_raw:
        sys.exit(1)

    # Transpose
    print(f"  Transposing {len(frames_raw)} frames H→V...")
    frames_v = [transpose_h_to_v(f) for f in frames_raw]

    # Verify transposed size
    for i, fv in enumerate(frames_v):
        assert len(fv) == 1024, f"Frame {i}: expected 1024, got {len(fv)}"

    # Generate outputs
    os.makedirs(h_dir, exist_ok=True)
    os.makedirs(c_dir, exist_ok=True)
    h_path = os.path.join(h_dir, f"{prefix}.h")
    c_path = os.path.join(c_dir, f"{prefix}.c")
    write_c_header(h_path, prefix, len(frames_v))
    write_c_source(c_path, prefix, frames_v)

    total_kb = sum(len(f) for f in frames_v) / 1024
    print(f"  Done! {len(frames_v)} frames, {total_kb:.1f} KB total")

if __name__ == '__main__':
    main()
