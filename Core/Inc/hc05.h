/**
 * @file    hc05.h
 * @brief   HC-05 蓝牙模块 — 单字符命令 (9600 8N1)
 *
 * 命令: t=摸头 f=喂食 z=睡觉 s=状态 h=帮助
 *       1=查询温度 2=查询温度+亮度
 *       3/4=高温阈值±50 5/6=暗光阈值±50
 */
#ifndef __HC05_H
#define __HC05_H

#include "main.h"
#include "pet_fsm.h"

void HC05_Init(void);
void HC05_Printf(const char *fmt, ...);
PetCmd_t HC05_Process(void);

#endif
