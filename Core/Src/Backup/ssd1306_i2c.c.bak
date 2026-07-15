/**
 * @file    ssd1306_i2c.c
 * @brief   SSD1306 OLED I2C 驱动实现
 *
 * 显示原理：SSD1306 采用页面寻址模式，屏幕分为 8 页（Page 0-7），
 * 每页 8 像素高 × 128 像素宽。一个字节控制一列中 8 个垂直像素，
 * 字节的 bit0 在最上面，bit7 在最下面。
 *
 * 通信格式：I2C 写入时先发控制字节（0x00=命令, 0x40=数据），再发实际内容。
 */

#include "ssd1306_i2c.h"
#include "i2c.h"
#include <string.h>

/* ===================== 全局缓冲 ===================== */
static uint8_t g_ssd1306_buffer[SSD1306_BUFSIZE];  /* 128×64 = 1024 字节 */

/* ===================== 底层 I2C 操作 ===================== */

void SSD1306_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd};  /* 控制字节 0x00 表示命令 */
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, buf, 2, 10);
}

void SSD1306_WriteData(uint8_t data)
{
    uint8_t buf[2] = {0x40, data};  /* 控制字节 0x40 表示数据 */
    HAL_I2C_Master_Transmit(&hi2c1, SSD1306_I2C_ADDR, buf, 2, 10);
}

/* ===================== 初始化 ===================== */

void SSD1306_Init(void)
{
    /* SSD1306 初始化序列（数据手册推荐） */
    SSD1306_WriteCmd(0xAE);  /* 关闭显示 */

    SSD1306_WriteCmd(0xD5);  /* 设置时钟分频 / 振荡频率 */
    SSD1306_WriteCmd(0x80);  /* 默认值：分频=1, 频率≈370kHz */

    SSD1306_WriteCmd(0xA8);  /* 设置多路复用比 */
    SSD1306_WriteCmd(0x3F);  /* 64 行 */

    SSD1306_WriteCmd(0xD3);  /* 设置显示偏移 */
    SSD1306_WriteCmd(0x00);  /* 不偏移 */

    SSD1306_WriteCmd(0x40);  /* 设置显示起始行 = 0 */

    SSD1306_WriteCmd(0x8D);  /* 电荷泵设置 */
    SSD1306_WriteCmd(0x14);  /* 启用电荷泵（3.3V 供电必须开启） */

    SSD1306_WriteCmd(0x20);  /* 内存寻址模式 */
    SSD1306_WriteCmd(0x02);  /* 页面寻址模式 (Page Addressing) */

    SSD1306_WriteCmd(0xA1);  /* 段重映射：列 127 = SEG0（水平翻转） */
    SSD1306_WriteCmd(0xC8);  /* COM 扫描方向：COM[N-1]→COM0（垂直翻转） */

    SSD1306_WriteCmd(0xDA);  /* COM 引脚硬件配置 */
    SSD1306_WriteCmd(0x12);  /* 备选引脚配置（128x64 专用） */

    SSD1306_WriteCmd(0x81);  /* 对比度 */
    SSD1306_WriteCmd(0xCF);  /* 对比度值（0-255，默认 0x7F） */

    SSD1306_WriteCmd(0xD9);  /* 预充电周期 */
    SSD1306_WriteCmd(0xF1);  /* Phase1=1, Phase2=15 */

    SSD1306_WriteCmd(0xDB);  /* VCOMH 电压 */
    SSD1306_WriteCmd(0x40);  /* ~0.77×VCC */

    SSD1306_WriteCmd(0xA4);  /* 恢复显示 RAM 内容（非全亮） */
    SSD1306_WriteCmd(0xA6);  /* 正常显示（不反色） */

    SSD1306_WriteCmd(0x2E);  /* 停止滚动 */

    SSD1306_WriteCmd(0xAF);  /* 开启显示 */

    /* 清空缓冲 */
    SSD1306_ClearBuffer();
    SSD1306_Refresh();
}

/* ===================== 显示刷新 ===================== */

void SSD1306_Refresh(void)
{
    /* 逐页刷新：设置页地址 → 列起始 → 发送 128 字节 */
    for (uint8_t page = 0; page < SSD1306_PAGES; page++)
    {
        SSD1306_WriteCmd(0xB0 | page);      /* 设置页地址 0-7 */
        SSD1306_WriteCmd(0x00);             /* 设置列低地址 = 0 */
        SSD1306_WriteCmd(0x10);             /* 设置列高地址 = 0 */

        /* 一次性发送该页 128 字节数据 */
        uint8_t *p = &g_ssd1306_buffer[page * SSD1306_WIDTH];
        for (uint8_t col = 0; col < SSD1306_WIDTH; col++)
        {
            SSD1306_WriteData(p[col]);
        }
    }
}

/* ===================== 缓冲操作 ===================== */

void SSD1306_ClearBuffer(void)
{
    memset(g_ssd1306_buffer, 0x00, SSD1306_BUFSIZE);
}

void SSD1306_FillBuffer(void)
{
    memset(g_ssd1306_buffer, 0xFF, SSD1306_BUFSIZE);
}

uint8_t* SSD1306_GetBuffer(void)
{
    return g_ssd1306_buffer;
}

/* ===================== 像素操作 ===================== */

/**
 * @brief 在缓冲中设置一个像素点
 * @param x  列 (0-127)
 * @param y  行 (0-63)
 * @param on 1=点亮, 0=熄灭
 *
 * 字节内部排列（垂直方向，bit0=上）：
 *   列 x, 页 page = y/8
 *   bit(y % 8) 控制该像素
 *   bit0 = 该页最上像素, bit7 = 该页最下像素
 */
void SSD1306_SetPixel(uint8_t x, uint8_t y, uint8_t on)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    uint8_t page = y / 8;
    uint8_t bit  = y % 8;
    uint16_t idx = page * SSD1306_WIDTH + x;

    if (on)
        g_ssd1306_buffer[idx] |=  (1 << bit);
    else
        g_ssd1306_buffer[idx] &= ~(1 << bit);
}
