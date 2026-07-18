/**
 * @file    aht20.h
 * @brief   AHT20 温湿度传感器驱动 — I²C 接口
 *
 * 特性：
 *   - I²C 地址：0x38（7-bit）
 *   - 温度精度：±0.3°C
 *   - 湿度精度：±2%RH
 *   - 可与 SSD1306 OLED 共用 I²C1 总线（地址不冲突：0x3C vs 0x38）
 *
 * 接线：
 *   AHT20 VCC → 3.3V
 *   AHT20 GND → GND
 *   AHT20 SCL → PB6 (I2C1_SCL)
 *   AHT20 SDA → PB7 (I2C1_SDA)
 */
#ifndef __AHT20_H
#define __AHT20_H

#include "main.h"

/** 传感器数据 */
typedef struct {
    float temperature;   /* 温度 ℃ */
    float humidity;      /* 湿度 %RH */
    uint8_t valid;       /* 数据有效标志 */
} AHT20_Data_t;

/** 初始化 AHT20（发送校准命令，约 40ms） */
void AHT20_Init(void);

/**
 * @brief 触发一次测量并读取结果
 * @note  测量周期约 80ms，调用后会阻塞等待
 * @return 最新温湿度数据
 */
AHT20_Data_t AHT20_Read(void);

/**
 * @brief 返回最近一次读取的数据（不触发新测量）
 */
AHT20_Data_t AHT20_GetLatest(void);

#endif /* __AHT20_H */
