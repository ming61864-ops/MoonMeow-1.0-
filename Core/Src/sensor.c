/**
 * @file    sensor.c
 * @brief   传感器驱动 - EMA滑动平均 + 可配置阈值
 */
#include "sensor.h"
#include "adc.h"
#include <stdio.h>

/* 默认阈值 (可通过 Flash 改写) */
uint16_t g_ldr_dark   = 3000;
uint16_t g_ldr_bright = 1200;
uint16_t g_ntc_hot    = 1300;
uint16_t g_ntc_cold   = 2800;

#define TIMEOUT     100
#define EMA_ALPHA   3   /* 1/8 = 0.125, 对应 >> 3 */

static uint16_t g_ldr = 0;
static uint16_t g_ntc = 0;

/* 滑动平均状态 */
static uint16_t g_ldr_ema = 0;
static uint16_t g_ntc_ema = 0;
static uint8_t  g_ema_init = 0;  /* 首次直接用原始值, 避免从0上升的延迟 */

/* ---- 用HAL读单个通道 ---- */
static uint16_t hal_read_ch(uint32_t channel)
{
    uint16_t val = 0;

    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, TIMEOUT) == HAL_OK) {
        val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);

    return val;
}

/* EMA 滤波: filtered = (filtered * (8-alpha) + new * alpha) / 8 */
static uint16_t ema_filter(uint16_t prev, uint16_t new_val)
{
    return (uint16_t)(((uint32_t)prev * (8 - EMA_ALPHA) + (uint32_t)new_val * EMA_ALPHA) >> 3);
}

void Sensor_Init(void)
{
    printf("[SENSOR] EMA filter ready, thresholds: LDR=%d/%d NTC=%d/%d\r\n",
           g_ldr_dark, g_ldr_bright, g_ntc_hot, g_ntc_cold);
}

void Sensor_Update(void)
{
    uint16_t raw_ldr = hal_read_ch(ADC_CHANNEL_0);
    uint16_t raw_ntc = hal_read_ch(ADC_CHANNEL_1);

    if (!g_ema_init) {
        g_ldr_ema = raw_ldr;
        g_ntc_ema = raw_ntc;
        g_ema_init = 1;
    } else {
        g_ldr_ema = ema_filter(g_ldr_ema, raw_ldr);
        g_ntc_ema = ema_filter(g_ntc_ema, raw_ntc);
    }

    g_ldr = g_ldr_ema;
    g_ntc = g_ntc_ema;
}

uint16_t Sensor_GetLDR(void)  { return g_ldr; }
uint16_t Sensor_GetNTC(void)  { return g_ntc; }

LightLevel_t Sensor_GetLight(void)
{
    if (g_ldr > g_ldr_dark)   return LIGHT_DARK;
    if (g_ldr < g_ldr_bright) return LIGHT_BRIGHT;
    return LIGHT_NORMAL;
}

TempLevel_t Sensor_GetTemp(void)
{
    if (g_ntc < g_ntc_hot)  return TEMP_HOT;
    if (g_ntc > g_ntc_cold) return TEMP_COLD;
    return TEMP_COMFORT;
}
