/**
 * @file    ssd1306_i2c.h
 * @brief   SSD1306 OLED I2C 驱动 - 0.96寸 128x64 白色像素
 */

#ifndef __SSD1306_I2C_H
#define __SSD1306_I2C_H

#include "main.h"

/* ===================== 分辨率 ===================== */
#define SSD1306_WIDTH    128
#define SSD1306_HEIGHT   64
#define SSD1306_PAGES    8       /* 64 / 8 = 8 页 */
#define SSD1306_BUFSIZE  (SSD1306_WIDTH * SSD1306_PAGES)  /* 1024 字节 */

/* ===================== I2C 地址 ===================== */
/* 模块默认地址 0x3C (SA0=GND), 若接 VCC 则为 0x3D */
#define SSD1306_I2C_ADDR  (0x3C << 1)  /* HAL 需要左移1位 */

/* ===================== 基础 API ===================== */

/** 初始化 SSD1306，发送命令序列完成配置 */
void SSD1306_Init(void);

/** 将内部缓冲刷新到 OLED 显示 */
void SSD1306_Refresh(void);

/** 清空内部缓冲（不刷新） */
void SSD1306_ClearBuffer(void);

/** 填充整个缓冲（全屏点亮） */
void SSD1306_FillBuffer(void);

/** 获取内部缓冲指针（供图形引擎直接操作） */
uint8_t* SSD1306_GetBuffer(void);

/** 设置像素 (x: 0-127, y: 0-63) */
void SSD1306_SetPixel(uint8_t x, uint8_t y, uint8_t on);

/* ===================== 底层 I2C 通信 ===================== */
void SSD1306_WriteCmd(uint8_t cmd);
void SSD1306_WriteData(uint8_t data);

#endif /* __SSD1306_I2C_H */
