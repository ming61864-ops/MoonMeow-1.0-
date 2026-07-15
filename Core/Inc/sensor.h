/**
 * @file    sensor.h
 * @brief   环境传感器 - 滑动平均 + 可配置阈值
 */
#ifndef __SENSOR_H
#define __SENSOR_H

#include "main.h"

typedef enum {
    LIGHT_DARK   = 0,
    LIGHT_NORMAL = 1,
    LIGHT_BRIGHT = 2
} LightLevel_t;

typedef enum {
    TEMP_COLD    = 0,
    TEMP_COMFORT = 1,
    TEMP_HOT     = 2
} TempLevel_t;

/* 阈值 (可通过 Flash 改写) */
extern uint16_t g_ldr_dark;
extern uint16_t g_ldr_bright;
extern uint16_t g_ntc_hot;
extern uint16_t g_ntc_cold;

void Sensor_Init(void);
void Sensor_Update(void);

uint16_t Sensor_GetLDR(void);
uint16_t Sensor_GetNTC(void);
LightLevel_t Sensor_GetLight(void);
TempLevel_t  Sensor_GetTemp(void);

#endif
