/**
 * @file    buzzer.h
 * @brief   有源蜂鸣器 - PA2 GPIO/TIM2 PWM 音量控制
 */
#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

void Buzzer_Init(void);

/** 发声 duration_ms 毫秒 (用默认音量) */
void Buzzer_Beep(uint16_t duration_ms);

/** 每 10ms 调一次 */
void Buzzer_Update(void);

typedef enum {
    MELODY_HAPPY,
    MELODY_SURPRISE,
    MELODY_ANNOYED,
    MELODY_CLICK
} Melody_t;

void Buzzer_PlayMelody(Melody_t melody);

#endif
