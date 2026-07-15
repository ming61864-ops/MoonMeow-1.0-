#!/usr/bin/env python3
"""
月薪猫表情 - 48x48 猫脸, 三角耳+胡须
"""
import math
W, H = 48, 48
PAGES = H // 8

def new(): return [[0]*W for _ in range(H)]
def circle(cx,cy,r):
    return {(x,y) for y in range(max(0,cy-r),min(H,cy+r+1))
            for x in range(max(0,cx-r),min(W,cx+r+1))
            if (x-cx)**2+(y-cy)**2<=r**2}
def ring(cx,cy,r,t=1): return circle(cx,cy,r)-circle(cx,cy,r-t)
def dot(cx,cy,r=2): return circle(cx,cy,r)
def filled_circle(cx,cy,r): return circle(cx,cy,r)
def line_h(y,x0,x1): return {(x,y) for x in range(x0,x1+1)}
def line_v(x,y0,y1): return {(x,y) for y in range(y0,y1+1)}
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
def triangle(x0,y0,x1,y1,x2,y2):
    pts=set()
    for y in range(min(y0,y1,y2),max(y0,y1,y2)+1):
        for x in range(min(x0,x1,x2),max(x0,x1,x2)+1):
            d1=(x1-x0)*(y-y0)-(y1-y0)*(x-x0)
            d2=(x2-x1)*(y-y1)-(y2-y1)*(x-x1)
            d3=(x0-x2)*(y-y2)-(y0-y2)*(x-x2)
            if (d1>=0 and d2>=0 and d3>=0) or (d1<=0 and d2<=0 and d3<=0):
                if 0<=x<W and 0<=y<H: pts.add((x,y))
    return pts
def draw(cv,pts):
    for x,y in pts:
        if 0<=x<W and 0<=y<H: cv[y][x]=1

def draw_cat_face(cv):
    """猫脸基底: 圆头+三角耳+胡须"""
    # 圆脸
    draw(cv,circle(24,26,20))
    # 左三角耳
    draw(cv,triangle(5,20, 18,2, 18,18))
    draw(cv,triangle(7,18, 16,5, 16,16))
    # 右三角耳
    draw(cv,triangle(43,20, 30,2, 30,18))
    draw(cv,triangle(41,18, 32,5, 32,16))
    # 胡须 (左)
    draw(cv,line_h(26,1,8)); draw(cv,line_h(28,1,8)); draw(cv,line_h(30,1,8))
    # 胡须 (右)
    draw(cv,line_h(26,40,47)); draw(cv,line_h(28,40,47)); draw(cv,line_h(30,40,47))

frames={}

# ===== IDLE: ●●— =====
c=new();draw_cat_face(c)
draw(c,ring(16,20,7,2));draw(c,ring(32,20,7,2))
draw(c,dot(16,20,2));draw(c,dot(32,20,2))
draw(c,line_h(37,20,28));draw(c,dot(24,32,2))
frames['idle_f0']=[r[:] for r in c]

c=new();draw_cat_face(c)
draw(c,ring(16,20,7,2));draw(c,ring(32,20,7,2))
draw(c,line_h(20,10,22));draw(c,line_h(20,26,38))
draw(c,line_h(37,20,28));draw(c,dot(24,32,2))
frames['idle_f1']=[r[:] for r in c]

# ===== HAPPY: ^^⌣ =====
c=new();draw_cat_face(c)
draw(c,arc(16,22,6,math.pi*0.8,math.pi*1.2,200))
draw(c,arc(32,22,6,math.pi*0.8,math.pi*1.2,200))
draw(c,arc(24,30,10,math.pi*0.15,math.pi*0.85,200))
draw(c,dot(24,32,2))
frames['happy_f0']=[r[:] for r in c]

c=new();draw_cat_face(c)
draw(c,arc(16,23,6,math.pi*0.8,math.pi*1.2,200))
draw(c,arc(32,23,6,math.pi*0.8,math.pi*1.2,200))
draw(c,arc(24,30,12,math.pi*0.12,math.pi*0.88,200))
draw(c,dot(24,32,2))
frames['happy_f1']=[r[:] for r in c]

# ===== SLEEP: ——Z =====
c=new();draw_cat_face(c)
draw(c,line_h(20,10,22));draw(c,line_h(20,26,38))
draw(c,dot(24,32,2));draw(c,filled_circle(24,39,1))
# Zzz
draw(c,line(35,6,41,6));draw(c,line_v(39,6,12));draw(c,line(35,12,41,12))
frames['sleep_f0']=[r[:] for r in c]

c=new();draw_cat_face(c)
draw(c,line_h(20,10,22));draw(c,line_h(20,26,38))
draw(c,dot(24,32,2));draw(c,filled_circle(24,39,2))
draw(c,line(35,4,42,4));draw(c,line_v(40,4,10));draw(c,line(35,10,42,10))
frames['sleep_f1']=[r[:] for r in c]

# ===== ANNOYED: ╲╱⌢ =====
c=new();draw_cat_face(c)
draw(c,line(6,12,22,18));draw(c,line(26,18,42,12))
draw(c,ring(16,24,5,2));draw(c,ring(32,24,5,2))
draw(c,dot(16,24,2));draw(c,dot(32,24,2))
draw(c,arc(24,46,10,math.pi*1.15,math.pi*1.85,200))
draw(c,dot(24,32,2))
frames['annoyed_f0']=[r[:] for r in c]

c=new();draw_cat_face(c)
draw(c,line(6,10,22,18));draw(c,line(26,18,42,10))
draw(c,ring(16,24,5,2));draw(c,ring(32,24,5,2))
draw(c,dot(16,24,1));draw(c,dot(32,24,1))
draw(c,arc(24,46,10,math.pi*1.15,math.pi*1.85,200))
draw(c,dot(24,32,2))
frames['annoyed_f1']=[r[:] for r in c]

# ===== SURPRISE: ◎◎○ =====
c=new();draw_cat_face(c)
draw(c,ring(16,20,9,2));draw(c,ring(32,20,9,2))
draw(c,dot(16,20,4));draw(c,dot(32,20,4))
draw(c,ring(24,39,5,2))
frames['surprise_f0']=[r[:] for r in c]

c=new();draw_cat_face(c)
draw(c,ring(16,20,9,2));draw(c,ring(32,20,9,2))
draw(c,dot(16,20,3));draw(c,dot(32,20,3))
draw(c,ring(24,39,4,2))
frames['surprise_f1']=[r[:] for r in c]

# ===== Output =====
def to_c(cv,name):
    buf=[]
    for p in range(PAGES):
        for col in range(W):
            b=0
            for bit in range(8):
                y=p*8+bit
                if y<H and cv[y][col]: b|=(1<<bit)
            buf.append(b)
    lines=[f'const uint8_t {name}[SPRITE_SIZE] = {{']
    for i in range(0,len(buf),12):
        lines.append('\t'+', '.join(f'0x{b:02X}' for b in buf[i:i+12])+',')
    lines.append('};')
    return '\n'.join(lines)

out=['#include "pet_sprites.h"','']
for name in sorted(frames.keys()):
    c=frames[name]
    out.append(to_c(c,name));out.append('')

with open('D:/UserData/Desktop/stm32dog/Core/Src/pet_sprites.c','w') as f:
    f.write('\n'.join(out))

# Update header to use correct names
hdr='''/**
 * @file    pet_sprites.h
 * @brief   月薪猫48x48点阵
 */
#ifndef __PET_SPRITES_H
#define __PET_SPRITES_H
#include "main.h"
#define SPRITE_W  48
#define SPRITE_H  48
#define SPRITE_SIZE  (SPRITE_W * SPRITE_H / 8)
extern const uint8_t sprite_idle_f0[SPRITE_SIZE];
extern const uint8_t sprite_idle_f1[SPRITE_SIZE];
extern const uint8_t sprite_happy_f0[SPRITE_SIZE];
extern const uint8_t sprite_happy_f1[SPRITE_SIZE];
extern const uint8_t sprite_sleep_f0[SPRITE_SIZE];
extern const uint8_t sprite_sleep_f1[SPRITE_SIZE];
extern const uint8_t sprite_annoyed_f0[SPRITE_SIZE];
extern const uint8_t sprite_annoyed_f1[SPRITE_SIZE];
extern const uint8_t sprite_surprise_f0[SPRITE_SIZE];
extern const uint8_t sprite_surprise_f1[SPRITE_SIZE];
#endif
'''
with open('D:/UserData/Desktop/stm32dog/Core/Inc/pet_sprites.h','w') as f:
    f.write(hdr)

print('Generated cat sprites!')
