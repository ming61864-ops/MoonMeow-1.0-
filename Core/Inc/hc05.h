/**
 * @file    hc05.h
 * @brief   HC-05 蓝牙模块 — USART2 透传, 9600 8N1
 *
 * 接线: HC-05 TXD → PA3 (USART2_RX), HC-05 RXD → PA2 (USART2_TX)
 */
#ifndef __HC05_H
#define __HC05_H

#include "main.h"

void HC05_Init(void);

/**
 * @brief 从 USART2 读取一个字节 (非阻塞)
 * @return 0~255=有效字节, -1=无数据
 */
int HC05_GetChar(void);

#endif
