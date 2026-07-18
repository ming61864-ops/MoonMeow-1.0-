/**
 * @file    buzzer.c
 * @brief   有源蜂鸣器(低电平触发) - GPIO直接开关
 *
 * 有源蜂鸣器自带振荡器, 通电就响.
 * PB0: HIGH=静音, LOW=发声.
 * (从 PA2 迁移到 PB0, 释放 PA2 给 USART2_TX 接 HC-05)
 */
#include "buzzer.h"
#include "gpio.h"

static uint32_t g_off_tick = 0;
static void melody_update(void);  /* 前置声明 */

static void buzzer_on(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  /* LOW=响 */
}

static void buzzer_off(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);    /* HIGH=静音 */
}

void Buzzer_Init(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_0;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &g);
    buzzer_off();  /* 默认静音 */
}

void Buzzer_Beep(uint16_t duration_ms)
{
    buzzer_on();
    g_off_tick = HAL_GetTick() + duration_ms;
}

void Buzzer_Update(void)
{
    if (g_off_tick && HAL_GetTick() >= g_off_tick) {
        buzzer_off();
        g_off_tick = 0;
    }
    melody_update();
}

/* ==================== 旋律 ==================== */

#define B  120
#define H  (B/2)

static const uint16_t melody_happy[]    = { B, H, B, H, B*2, 0 };
static const uint16_t melody_surprise[] = { H, 0 };
static const uint16_t melody_annoyed[]  = { B*2, H, B*2, 0 };
static const uint16_t melody_click[]    = { 30, 0 };

static const uint16_t *g_melody = NULL;
static uint8_t  g_idx = 0;
static uint32_t g_next = 0;
static uint8_t  g_note_on = 0;

void Buzzer_PlayMelody(Melody_t m)
{
    switch (m) {
        case MELODY_HAPPY:    g_melody = melody_happy;    break;
        case MELODY_SURPRISE: g_melody = melody_surprise; break;
        case MELODY_ANNOYED:  g_melody = melody_annoyed;  break;
        case MELODY_CLICK:    g_melody = melody_click;    break;
    }
    g_idx = 0; g_next = 0; g_note_on = 0;
}

static void melody_update(void)
{
    if (!g_melody) return;
    uint32_t now = HAL_GetTick();
    if (now < g_next) return;

    uint16_t dur = g_melody[g_idx];
    if (dur == 0) { g_melody = NULL; buzzer_off(); return; }

    g_note_on = !g_note_on;
    if (g_note_on) Buzzer_Beep(dur);
    else           buzzer_off();

    g_next = now + dur;
    if (!g_note_on) g_idx++;
}
