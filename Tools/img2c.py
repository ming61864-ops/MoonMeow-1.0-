#!/usr/bin/env python3
"""
ASCII art → SSD1306 点阵 C数组 转换器
输入: 32x32 的文本 (空格=灭, 任意非空格=亮)
输出: C 数组, SSD1306 垂直字节格式
"""
import sys, os

W, H = 32, 32

def parse_ascii(text):
    """将文本转为布尔矩阵 [row][col]"""
    lines = text.strip().split('\n')
    rows = []
    for ln in lines:
        ln = ln.rstrip()
        row = [1 if i < len(ln) and ln[i] != ' ' else 0 for i in range(W)]
        rows.append(row)
    if len(rows) < H:
        rows += [[0]*W for _ in range(H - len(rows))]
    return rows[:H]

def to_ssd1306(rows):
    """转为SSD1306格式: page(0..3) × col(0..31), 每字节8垂直像素"""
    pages = H // 8
    buf = []
    for p in range(pages):
        for c in range(W):
            byte = 0
            for b in range(8):
                y = p * 8 + b
                if y < H and rows[y][c]:
                    byte |= (1 << b)
            buf.append(byte)
    return buf

# ===== 小狗表情 ASCII 艺术 (32x32) =====

sprite_idle_f0 = """
            @@
          @@@@@@
         @@@@@@@@
        @@@@@@@@@@
       @@@@@@  @@@@
      @@@@@@    @@@@
     @@@@@@      @@@@
    @@@@@@  @@@@  @@@@
   @@@@@@@@@@@@@@@@@@@@
   @@@@@@@@@@@@@@@@@@@@
  @@@  @@@@@@@@@@  @@@
  @@  @@@@@@@@@@@@  @@
  @@  @@  @@@@  @@  @@
  @@@@@@  @@@@  @@@@@@
  @@@@@@  @@@@  @@@@@@
  @@@@@@  @@@@  @@@@@@
   @@@@  @@@@@@  @@@@
   @@@  @@@@@@@@  @@@
    @  @@@@@@@@@@  @
       @@  @@  @@
       @@@@@@@@@@@@
       @@@ @@@ @@@
       @@  @@@  @@
       @@  @@@  @@
       @@@@@@@@@@@
        @@@@@@@@@
         @@@@@@@
          @@@@@
           @@@
            @
"""
sprite_idle_f1 = sprite_idle_f0  # 待机2帧, 先用同样

sprite_happy_f0 = """
            @@
          @@@@@@
         @@@@@@@@
        @@@@@@@@@@
       @@@@@@  @@@@
      @@@@@@    @@@@
     @@@@@@      @@@@
    @@@@@@  @@@@  @@@@
   @@@@@@@@@@@@@@@@@@@@
   @@@@@@@@@@@@@@@@@@@@
  @@@  @@@@@@@@@@  @@@
  @@  @@@@@@@@@@@@  @@
  @@  @@  @@@@  @@  @@
  @@@@@@  @@@@  @@@@@@
  @@@@@@  @@@@  @@@@@@
   @@@@  @@@@@@  @@@@
   @@@@  @@@@@@  @@@@
    @@  @@@@@@@@  @@
         @@@@@@
      @@@ @@@@ @@@
     @@@  @@@@  @@@
     @@   @@@@   @@
     @@@@@@@@@@@@@@
      @@@@@@@@@@@@
        @@@@@@@@@
         @@@@@@@
          @@@@@
           @@@
            @
"""
sprite_happy_f1 = sprite_happy_f0  # 同样

sprite_sleep_f0 = """
            @@
          @@@@@@
         @@@@@@@@
        @@@@@@@@@@
       @@@@@@  @@@@
      @@@@@@    @@@@
     @@@@@@      @@@@
    @@@@@@  @@@@  @@@@
   @@@@@@@@@@@@@@@@@@@@
   @@@@@@@@@@@@@@@@@@@@
  @@@  @@@@@@@@@@  @@@
  @@   @@@@@@@@@@   @@
  @@   @@      @@   @@
  @@@@@@        @@@@@@
  @@@@@@        @@@@@@
  @@@@@@        @@@@@@
   @@@@  @@@@@@  @@@@
   @@@  @@@@@@@@  @@@
    @  @@@@@@@@@@  @
       @@  @@  @@
       @@@ @@@ @@@
       @@  @@@  @@
       @@  @@@  @@
       @@@@    @@@@
        @@@@@@@@@@
         @@@@@@@
          @@@@@
           @@@
            @
"""
sprite_sleep_f1 = sprite_sleep_f0

sprite_annoyed_f0 = """
            @@
          @@@@@@
         @@@@@@@@
        @@@@@@@@@@
       @@@@@@  @@@@
      @@@@@@    @@@@
     @@@@@@      @@@@
    @@@@@@  @@@@  @@@@
   @@@@@@@@@@@@@@@@@@@@
   @@@@@@@@@@@@@@@@@@@@
  @@@  @@@@@@@@@@  @@@
  @@  @@@@@@@@@@@@  @@
  @@  @@      @@  @@  @
  @@@@@@  @@@@  @@@@@@
  @@@@@@  @@@@  @@@@@@
  @@@@@@  @@@@  @@@@@@
   @@@@          @@@@
   @@@  @@@@@@@@  @@@
    @  @@@@@@@@@@  @
         @@@@@@
        @@@@@@@@
       @@@@@@@@@@
       @@  @@@  @@
       @@  @@@  @@
       @@@@@@@@@@@
        @@@@@@@@@
         @@@@@@@
          @@@@@
           @@@
            @
"""
sprite_annoyed_f1 = sprite_annoyed_f0

sprite_surprise_f0 = """
            @@
          @@@@@@
         @@@@@@@@
        @@@@@@@@@@
       @@@@@@  @@@@
      @@@@@@    @@@@
     @@@@@@      @@@@
    @@@@@@  @@@@  @@@@
   @@@@@@@@@@@@@@@@@@@@
   @@@@@@@@@@@@@@@@@@@@
  @@@  @@@@@@@@@@  @@@
  @@  @@@@@@@@@@@@  @@
  @@  @@  @@@@  @@  @@
  @@@@@@  @@@@  @@@@@@
  @@@@@@  @@@@  @@@@@@
  @@@@@@  @@@@  @@@@@@
   @@@@  @@@@@@  @@@@
   @@@  @@@@@@@@  @@@
    @  @@@@@@@@@@  @
       @@  @@  @@
       @@@ @@@ @@@
       @@@@@@@@@@@@
       @@  @@@  @@
       @    @@    @
       @@@@@@@@@@@@
        @@@@@@@@@
         @@@@@@@
          @@@@@
           @@@
            @
"""
sprite_surprise_f1 = sprite_surprise_f0

# ===== 生成 C 文件 =====
sprites = [
    ("sprite_idle",     [sprite_idle_f0,     sprite_idle_f1]),
    ("sprite_happy",    [sprite_happy_f0,    sprite_happy_f1]),
    ("sprite_sleep",    [sprite_sleep_f0,    sprite_sleep_f1]),
    ("sprite_annoyed",  [sprite_annoyed_f0,  sprite_annoyed_f1]),
    ("sprite_surprise", [sprite_surprise_f0, sprite_surprise_f1]),
]

out = [
    '/**',
    ' * @file    pet_sprites.c',
    ' * @brief   小狗点阵动画数据 (自动生成)',
    ' */',
    '#include "pet_sprites.h"',
    ''
]

for name, frames in sprites:
    for fi, frame in enumerate(frames):
        buf = to_ssd1306(parse_ascii(frame))
        out.append(f'/* {name} frame {fi} */')
        out.append(f'const uint8_t {name}_f{fi}[SPRITE_SIZE] = {{')
        for i in range(0, len(buf), 16):
            chunk = ', '.join(f'0x{b:02X}' for b in buf[i:i+16])
            out.append(f'    {chr(9)}{chunk},')
        out.append('};')
        out.append('')

# 生成组合数组
for name, frames in sprites:
    n = len(frames)
    refs = ', '.join(f'{name}_f{i}' for i in range(n))
    out.append(f'const uint8_t {name}[{n}][SPRITE_SIZE] = {{')
    out.append(f'    {refs}')
    out.append('};')
    out.append('')

output_path = os.path.join(os.path.dirname(__file__) if '__file__' not in dir() else '.', 'pet_sprites_out.c')
# 直接写到目标路径
target = 'D:/UserData/Desktop/stm32dog/Core/Src/pet_sprites.c'
with open(target, 'w', encoding='utf-8') as f:
    f.write('\n'.join(out))
print(f'Generated: {target}')
print(f'Total size: {len(out)} lines')
