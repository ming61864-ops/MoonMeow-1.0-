/**
 * @file    gfx_engine.h
 * @brief   图形引擎 - 基于 SSD1306 128x64 缓冲
 */
#ifndef __GFX_ENGINE_H
#define __GFX_ENGINE_H

#include "main.h"

void GFX_Clear(void);                          /* 清屏 */
void GFX_Refresh(void);                        /* 刷新到OLED */
void GFX_SetPixel(uint8_t x, uint8_t y);       /* 画点 */
void GFX_ClrPixel(uint8_t x, uint8_t y);       /* 擦除点 */
void GFX_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void GFX_DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void GFX_FillRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/** 绘制单色位图 (1 bit/pixel, 垂直字节排列, 与SSD1306格式一致) */
void GFX_DrawBitmap(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                    const uint8_t *bitmap);

/** 绘制ASCII字符 (5x7 字体, 6x8 单元格) */
void GFX_DrawChar(uint8_t x, uint8_t y, char c);

/** 绘制字符串, 自动换行 */
void GFX_DrawString(uint8_t x, uint8_t y, const char *str);

#endif
