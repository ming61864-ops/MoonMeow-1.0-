#!/usr/bin/env python3
"""
超简洁几何表情 - 圆形脸+简单五官, 48x48
IDLE: ● ● —
HAPPY: ⌒ ⌒ ⌣
SLEEP: — — ○
ANNOYED: ╲ ╱ ⌢
SURPRISE: ◎ ◎ ○
"""
W, H = 48, 48
PAGES = H // 8

def new(): return [[0]*W for _ in range(H)]

def circle(cx, cy, r):
    return {(x,y) for y in range(max(0,cy-r), min(H,cy+r+1))
            for x in range(max(0,cx-r), min(W,cx+r+1))
            if (x-cx)**2+(y-cy)**2 <= r**2}
def ring(cx, cy, r, t=1): return circle(cx,cy,r)-circle(cx,cy,r-t)
def dot(cx, cy, r=2): return circle(cx, cy, r)
def filled_circle(cx,cy,r): return circle(cx,cy,r)
def line_h(y, x0, x1): return {(x,y) for x in range(x0, x1+1)}
def line_v(x, y0, y1): return {(x,y) for y in range(y0, y1+1)}
def line(x0,y0,x1,y1):
    pts=set();dx=abs(x1-x0);dy=-abs(y1-y0);sx=1 if x0<x1 else -1;sy=1 if y0<y1 else -1;e=dx+dy
    while 1:
        pts.add((x0,y0))
        if x0==x1 and y0==y1: break
        e2=2*e
        if e2>=dy: e+=dy;x0+=sx
        if e2<=dx: e+=dx;y0+=sy
    return pts
def arc(cx,cy,r,start,end,n=200):
    pts=set()
    for i in range(n):
        a=start+(end-start)*i/n
        x=int(cx+r*math.cos(a));y=int(cy-r*math.sin(a))
        if 0<=x<W and 0<=y<H: pts.add((x,y))
    return pts
def draw(pts):
    for x,y in pts:
        if 0<=x<W and 0<=y<H: c[y][x]=1

def to_c(name):
    buf=[]
    for p in range(PAGES):
        for col in range(W):
            b=0
            for bit in range(8):
                y=p*8+bit
                if y<H and c[y][col]: b|=(1<<bit)
            buf.append(b)
    lines=[f'const uint8_t {name}[SPRITE_SIZE] = {{']
    for i in range(0,len(buf),12):
        lines.append('\t'+', '.join(f'0x{b:02X}' for b in buf[i:i+12])+',')
    lines.append('};')
    return '\n'.join(lines)

frames={}
for tag in ['idle','happy','sleep','annoyed','surprise']:
    frames[tag]=[]

import math

# ===== IDLE: ● ● — =====
c=new()
draw(circle(24,24,22))           # head
draw(ring(16,18,7,2))            # left eye
draw(ring(32,18,7,2))            # right eye
draw(dot(16,18,2))               # left pupil
draw(dot(32,18,2))               # right pupil
draw(line_h(35,18,30))           # mouth
draw(ring(24,36,3))              # nose
frames['idle'].append([r[:] for r in c])

c=new()
draw(circle(24,24,22))
draw(ring(16,18,7,2)); draw(ring(32,18,7,2))
draw(line_h(18,9,23)); draw(line_h(18,25,39))  # blink
draw(line_h(35,18,30))
draw(ring(24,36,3))
frames['idle'].append([r[:] for r in c])

# ===== HAPPY: ^ ^ ⌣ =====
c=new()
draw(circle(24,24,22))
# ^^ eyes
draw(line(9,18,23,8)); draw(line(23,8,18,18))   # left ^
draw(line(25,8,39,18)); draw(line(39,18,30,8))   # right ^
# smile
draw(arc(24,24,12,math.pi*0.1,math.pi*0.9,200))
draw(ring(24,34,3))
frames['happy'].append([r[:] for r in c])

c=new()
draw(circle(24,24,22))
draw(line(9,18,23,8)); draw(line(23,8,18,18))
draw(line(25,8,39,18)); draw(line(39,18,30,8))
draw(arc(24,24,14,math.pi*0.1,math.pi*0.9,200))
draw(ring(24,34,3))
frames['happy'].append([r[:] for r in c])

# ===== SLEEP: — — z =====
c=new()
draw(circle(24,24,22))
draw(line_h(18,10,24)); draw(line_h(18,28,38))  # closed eyes
# nose + mouth
draw(ring(24,34,3))
draw(filled_circle(24,40,1))  # small mouth
# Zzz
draw(line(34,4,40,4)); draw(line_v(38,4,10)); draw(line(34,10,40,10))  # Z
frames['sleep'].append([r[:] for r in c])

c=new()
draw(circle(24,24,22))
draw(line_h(18,10,24)); draw(line_h(18,28,38))
draw(ring(24,34,3))
draw(filled_circle(24,40,2))  # mouth slightly open
draw(line(34,2,40,2)); draw(line_v(38,2,8)); draw(line(34,8,40,8))
frames['sleep'].append([r[:] for r in c])

# ===== ANNOYED: angry =====
c=new()
draw(circle(24,24,22))
# \ / eyebrows
draw(line(8,10,22,16)); draw(line(26,16,40,10))
# eyes
draw(ring(16,22,5,2)); draw(ring(32,22,5,2))
draw(dot(16,22,2)); draw(dot(32,22,2))
# frown
draw(arc(24,44,10,math.pi*1.1,math.pi*1.9,200))
draw(ring(24,34,3))
frames['annoyed'].append([r[:] for r in c])

c=new()
draw(circle(24,24,22))
draw(line(8,8,22,16)); draw(line(26,16,40,8))  # steeper brows
draw(ring(16,22,5,2)); draw(ring(32,22,5,2))
draw(dot(16,22,1)); draw(dot(32,22,1))  # smaller pupils
draw(arc(24,44,10,math.pi*1.1,math.pi*1.9,200))
draw(ring(24,34,3))
frames['annoyed'].append([r[:] for r in c])

# ===== SURPRISE: O O O =====
c=new()
draw(circle(24,24,22))
# big eyes
draw(ring(16,18,9,2)); draw(ring(32,18,9,2))
draw(dot(16,18,4)); draw(dot(32,18,4))
# O mouth
draw(ring(24,38,5,2))
frames['surprise'].append([r[:] for r in c])

c=new()
draw(circle(24,24,22))
draw(ring(16,18,9,2)); draw(ring(32,18,9,2))
draw(dot(16,18,3)); draw(dot(32,18,3))
draw(ring(24,38,4,2))
frames['surprise'].append([r[:] for r in c])

# ---- Output ----
out=['#include "pet_sprites.h"','']
for tag in ['idle','happy','sleep','annoyed','surprise']:
    for fi, f in enumerate(frames[tag]):
        c=f; out.append(to_c(f'sprite_{tag}_f{fi}')); out.append('')

with open('D:/UserData/Desktop/stm32dog/Core/Src/pet_sprites.c','w') as f:
    f.write('\n'.join(out))
print('Generated pet_sprites.c')
