/**
 * @file    pet_fsm.h
 * @brief   月薪喵情绪状态机 - 5状态版本 (IDLE/HAPPY/SLEEP/SURPRISE/HOT)
 */
#ifndef __PET_FSM_H
#define __PET_FSM_H

#include "main.h"

/** 动画索引 (对应5组全屏动画) */
typedef enum {
    ANIM_IDLE = 0,
    ANIM_HAPPY,
    ANIM_SLEEP,
    ANIM_SURPRISE,
    ANIM_HOT
} AnimSlot_t;

typedef enum {
    STATE_IDLE = 0,
    STATE_HAPPY,
    STATE_SLEEP,
    STATE_SURPRISE,
    STATE_HOT
} PetState_t;

/** 命令类型 (来自串口) */
typedef enum {
    CMD_NONE = 0,
    CMD_TOUCH,    /* 摸头 → SURPRISE */
    CMD_FEED,     /* 喂食 → HAPPY */
    CMD_SLEEP     /* 调试: 强制 SLEEP */
} PetCmd_t;

void PetFSM_Init(void);

/** 主更新: 输入传感器等级 + 命令, 输出动画/音效 */
void PetFSM_Update(uint8_t light_level, uint8_t temp_level, PetCmd_t cmd);

/** 获取当前情绪 */
PetState_t PetFSM_GetState(void);

/** 获取当前应播放的动画槽位 (0-4) */
uint8_t PetFSM_GetAnimSlot(void);

/** 获取当前应播放的音效 (-1=无) */
int8_t PetFSM_GetMelody(void);

#endif
