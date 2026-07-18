/**
 * @file    aht20.c
 * @brief   AHT20 温湿度传感器驱动实现
 *
 * 数据手册关键时序：
 *   1. 上电后等待 40ms，发送初始化命令 0xBE（校准）
 *   2. 触发测量：发送 0xAC, 0x33, 0x00
 *   3. 等待 80ms（测量完成），读取 7 字节
 *   4. 检查状态字节 bit[7]=0（数据就绪），bit[3]=0（已校准）
 *
 * 数据解析（大端序）：
 *   Byte0: 状态
 *   Byte1: 湿度高 8 位
 *   Byte2: 湿度低 12 位中的高 4 位 | 温度高 4 位中的低 4 位
 *   Byte3: 温度中 8 位
 *   Byte4: 温度低 4 位（高 4 位）
 *   Byte5: CRC（忽略）
 *
 *   湿度(20bit) = (Byte1<<12) | (Byte2<<4) | (Byte3>>4)
 *   温度(20bit) = ((Byte3&0x0F)<<16) | (Byte4<<8) | Byte5
 *
 *   湿度% = (raw_humi / 1048576.0) * 100.0
 *   温度℃ = (raw_temp / 1048576.0) * 200.0 - 50.0
 */

#include "aht20.h"
#include "i2c.h"
#include <stdio.h>

/* AHT20 I²C 地址（7-bit，HAL 使用时左移 1 位） */
#define AHT20_ADDR      0x38
#define AHT20_ADDR_HAL  (AHT20_ADDR << 1)

/* 命令 */
#define AHT20_CMD_INIT      0xBE   /* 初始化/校准 */
#define AHT20_CMD_TRIGGER   0xAC   /* 触发测量 */
#define AHT20_INIT_1        0x08
#define AHT20_INIT_2        0x00
#define AHT20_TRIG_1        0x33
#define AHT20_TRIG_2        0x00

/* 状态位 */
#define AHT20_STATUS_BUSY   0x80   /* bit7=1 表示测量中 */
#define AHT20_STATUS_CAL    0x08   /* bit3=1 表示已校准 */

static AHT20_Data_t g_latest = {0.0f, 0.0f, 0};

/**
 * @brief 向 AHT20 发送 3 字节命令
 */
static uint8_t aht20_send_cmd(uint8_t cmd, uint8_t param1, uint8_t param2)
{
    uint8_t buf[3] = {cmd, param1, param2};
    return HAL_I2C_Master_Transmit(&hi2c1, AHT20_ADDR_HAL, buf, 3, 50);
}

/**
 * @brief 从 AHT20 读取 n 字节
 */
static uint8_t aht20_read(uint8_t *buf, uint8_t len)
{
    return HAL_I2C_Master_Receive(&hi2c1, AHT20_ADDR_HAL, buf, len, 50);
}

/**
 * @brief 读取 AHT20 状态字节
 */
static uint8_t aht20_get_status(void)
{
    uint8_t status = 0;
    aht20_read(&status, 1);
    return status;
}

/* ==================== API ==================== */

void AHT20_Init(void)
{
    printf("[AHT20] Initializing...\r\n");

    /* 上电后等待传感器稳定 */
    HAL_Delay(40);

    /* 读取状态，检查是否需要校准 */
    uint8_t status = aht20_get_status();
    printf("[AHT20] Status=0x%02X (busy=%d, cal=%d)\r\n",
           status,
           (status & AHT20_STATUS_BUSY) ? 1 : 0,
           (status & AHT20_STATUS_CAL)  ? 1 : 0);

    /* 发送初始化命令 */
    if (aht20_send_cmd(AHT20_CMD_INIT, AHT20_INIT_1, AHT20_INIT_2) != HAL_OK) {
        printf("[AHT20] Init cmd FAILED\r\n");
        return;
    }
    HAL_Delay(40);

    /* 再次检查状态 */
    status = aht20_get_status();
    printf("[AHT20] After init: Status=0x%02X\r\n", status);

    if (status & AHT20_STATUS_CAL) {
        printf("[AHT20] Calibrated OK\r\n");
    } else {
        printf("[AHT20] WARNING: Not calibrated! Check wiring.\r\n");
    }
}

AHT20_Data_t AHT20_Read(void)
{
    AHT20_Data_t result = {0.0f, 0.0f, 0};

    /* 1. 发送触发测量命令 */
    if (aht20_send_cmd(AHT20_CMD_TRIGGER, AHT20_TRIG_1, AHT20_TRIG_2) != HAL_OK) {
        printf("[AHT20] Trigger FAILED\r\n");
        return result;
    }

    /* 2. 等待测量完成（典型 80ms） */
    HAL_Delay(80);

    /* 3. 读取 7 字节数据 */
    uint8_t buf[7] = {0};
    if (aht20_read(buf, 7) != HAL_OK) {
        printf("[AHT20] Read FAILED\r\n");
        return result;
    }

    /* 4. 检查状态：bit7 应为 0（不忙） */
    if (buf[0] & AHT20_STATUS_BUSY) {
        printf("[AHT20] Data not ready (busy)\r\n");
        return result;
    }

    /* 5. 解析温湿度原始值（20bit） */
    uint32_t raw_humi = ((uint32_t)buf[1] << 12)
                      | ((uint32_t)buf[2] << 4)
                      | ((uint32_t)buf[3] >> 4);

    uint32_t raw_temp = (((uint32_t)buf[3] & 0x0F) << 16)
                      | ((uint32_t)buf[4] << 8)
                      |  (uint32_t)buf[5];

    /* 6. 转换为物理量 */
    float humidity    = (float)raw_humi / 1048576.0f * 100.0f;
    float temperature = (float)raw_temp / 1048576.0f * 200.0f - 50.0f;

    result.temperature = temperature;
    result.humidity    = humidity;
    result.valid       = 1;

    /* 更新缓存 */
    g_latest = result;

    return result;
}

AHT20_Data_t AHT20_GetLatest(void)
{
    return g_latest;
}
