#!/usr/bin/env python3
"""
程序化生成48x48小狗表情 - 使用几何绘图函数
"""

W, H = 48, 48
PAGES = H // 8

def new_canvas():
    return [[0]*W for _ in range(H)]

def circle(cx, cy, r):
    """返回圆内的 (x,y) 集合"""
    pts = set()
    for y in range(max(0,cy-r), min(H-1,cy+r)+1):
        for x in range(max(0,cx-r), min(W-1,cx+r)+1):
            if (x-cx)**2 + (y-cy)**2 <= r**2:
                pts.add((x,y))
    return pts

def filled_circle(cx, cy, r):
    return circle(cx, cy, r)

def ring(cx, cy, r, thickness=1):
    return circle(cx,cy,r) - circle(cx,cy,r-thickness)

def line_h(y, x0, x1):
    return {(x,y) for x in range(max(0,x0), min(W,x1+1))}

def line_v(x, y0, y1):
    return {(x,y) for y in range(max(0,y0), min(H,y1+1))}

def rect(x0, y0, x1, y1):
    pts = set()
    for y in range(y0, y1+1):
        for x in range(x0, x1+1):
            pts.add((x,y))
    return pts

def draw(pts, val=1):
    for x,y in pts:
        if 0<=x<W and 0<=y<H:
            canvas[y][x] = val

def to_c_array(name):
    buf = []
    for p in range(PAGES):
        for c in range(W):
            byte = 0
            for b in range(8):
                y = p*8+b
                if y<H and canvas[y][c]:
                    byte |= (1<<b)
            buf.append(byte)
    lines = [f'const uint8_t {name}[SPRITE_SIZE] = {{']
    for i in range(0, len(buf), 12):
        lines.append('\t' + ', '.join(f'0x{b:02X}' for b in buf[i:i+12]) + ',')
    lines.append('};')
    return '\n'.join(lines)

# ============================================================
# 通用狗脸基底 (圆形头 + 耳朵 + 鼻子)
# ============================================================
def draw_dog_face():
    global canvas
    canvas = new_canvas()

    # 头部: 椭圆形
    draw(circle(24, 26, 18))  # 头主体
    draw(circle(24, 28, 15))  # 内脸 (稍微调整)

    # 耳朵
    draw(circle(8, 14, 8))    # 左耳
    draw(circle(40, 14, 8))   # 右耳
    draw(circle(8, 14, 6))
    draw(circle(40, 14, 6))

# ---- IDLE: 圆眼 + 微笑 ----
canvas = None
draw_dog_face()
# 眼睛轮廓
draw(ring(17, 24, 5))
draw(ring(31, 24, 5))
# 瞳孔
draw(filled_circle(18, 24, 2))
draw(filled_circle(32, 24, 2))
# 鼻子
draw(filled_circle(24, 32, 2))
# 嘴 (小弧线)
draw(line_h(35, 20, 28))
draw(line_h(36, 18, 30))
idle_f0 = canvas[:]

# IDLE frame 2: 闭眼眨眼
canvas = None
draw_dog_face()
draw(ring(17, 24, 5))
draw(ring(31, 24, 5))
# 左眼闭合
draw(line_h(24, 12, 22))
# 右眼正常
draw(filled_circle(32, 24, 2))
draw(filled_circle(24, 32, 2))
draw(line_h(35, 20, 28))
draw(line_h(36, 18, 30))
idle_f1 = canvas[:]

# ---- HAPPY: ^^眼 + 大笑 ----
canvas = None
draw_dog_face()
# ^^ 眼睛 (两条斜线)
draw(line_h(22, 13, 18))
draw(line_h(21, 17, 20))
draw(line_h(22, 27, 33))
draw(line_h(21, 29, 33))
# 鼻子
draw(filled_circle(24, 32, 2))
# 大笑嘴
draw(line_h(36, 16, 32))
draw(line_h(37, 14, 34))
draw(line_h(35, 18, 30))
happy_f0 = canvas[:]

canvas = None
draw_dog_face()
draw(line_h(22, 13, 18))
draw(line_h(21, 17, 20))
draw(line_h(22, 27, 33))
draw(line_h(21, 29, 33))
draw(filled_circle(24, 32, 2))
draw(line_h(36, 16, 32))
draw(line_h(37, 15, 33))
draw(line_h(35, 19, 29))
happy_f1 = canvas[:]

# ---- SLEEP: 闭眼 + 小嘴 ----
canvas = None
draw_dog_face()
# 闭眼
draw(line_h(24, 12, 20))
draw(line_h(24, 28, 36))
# 鼻子
draw(filled_circle(24, 32, 2))
# 小嘴
draw(filled_circle(24, 37, 1))
draw(filled_circle(22, 37, 1))
draw(filled_circle(26, 37, 1))
# Zzz
draw(rect(34, 5, 38, 8))
draw(rect(36, 5, 40, 1))
draw(rect(34, 8, 36, 8))
sleep_f0 = canvas[:]

canvas = None
draw_dog_face()
draw(line_h(24, 12, 20))
draw(line_h(24, 28, 36))
draw(filled_circle(24, 32, 2))
# 嘴微张
draw(filled_circle(24, 37, 2))
draw(rect(34, 5, 38, 8))
draw(rect(36, 5, 40, 1))
draw(rect(34, 8, 36, 8))
sleep_f1 = canvas[:]

# ---- ANNOYED: 倒八眉 + 下弯嘴 ----
canvas = None
draw_dog_face()
# 眼睛
draw(ring(17, 24, 5))
draw(ring(31, 24, 5))
draw(filled_circle(18, 24, 2))
draw(filled_circle(32, 24, 2))
# 倒八眉 (斜线向下)
draw(line_h(17, 10, 25))
for i in range(10, 25):
    canvas[17][i] = 1
# 右眉
draw(line_h(17, 23, 38))
for i in range(23, 39):
    canvas[17][i] = 1
# 鼻子
draw(filled_circle(24, 32, 2))
# 下弯嘴
draw(line_h(37, 18, 22))
draw(line_h(36, 22, 26))
draw(line_h(37, 26, 30))
annoyed_f0 = canvas[:]

canvas = None
draw_dog_face()
draw(ring(17, 24, 4))
draw(ring(31, 24, 4))
draw(filled_circle(18, 24, 1))
draw(filled_circle(32, 24, 1))
# 倒八眉
for i in range(10, 25):
    canvas[17][i] = 1
for i in range(23, 39):
    canvas[17][i] = 1
draw(filled_circle(24, 32, 2))
draw(line_h(37, 18, 22))
draw(line_h(36, 22, 26))
draw(line_h(37, 26, 30))
annoyed_f1 = canvas[:]

# ---- SURPRISE: 大圆眼 + O嘴 ----
canvas = None
draw_dog_face()
# 大圆眼
draw(ring(17, 24, 7, 2))
draw(ring(31, 24, 7, 2))
draw(filled_circle(17, 24, 4))
draw(filled_circle(31, 24, 4))
# 鼻子
draw(filled_circle(24, 32, 2))
# O嘴
draw(ring(24, 40, 4))
surprise_f0 = canvas[:]

canvas = None
draw_dog_face()
draw(ring(17, 24, 7, 2))
draw(ring(31, 24, 7, 2))
draw(filled_circle(17, 24, 3))
draw(filled_circle(31, 24, 3))
draw(filled_circle(24, 32, 2))
draw(ring(24, 40, 3))
surprise_f1 = canvas[:]

# ==================== 输出 ====================
sprites = [
    ("sprite_idle",     [idle_f0, idle_f1]),
    ("sprite_happy",    [happy_f0, happy_f1]),
    ("sprite_sleep",    [sleep_f0, sleep_f1]),
    ("sprite_annoyed",  [annoyed_f0, annoyed_f1]),
    ("sprite_surprise", [surprise_f0, surprise_f1]),
]

out = [
    '/**',
    ' * @file    pet_sprites.c',
    ' * @brief   小狗48x48点阵 (程序化几何生成)',
    ' */',
    '#include "pet_sprites.h"',
    ''
]

for name, frames in sprites:
    for fi, frame in enumerate(frames):
        canvas = frame
        out.append(to_c_array(f'{name}_f{fi}'))
        out.append('')

target = 'D:/UserData/Desktop/stm32dog/Core/Src/pet_sprites.c'
with open(target, 'w') as f:
    f.write('\n'.join(out))
print(f'Generated {target}')
print(f'Sprites: {len(sprites)} emotions x 2 frames each')
