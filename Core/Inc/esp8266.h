/**
 * @file    esp8266.h
 * @brief   ESP-01S WiFi 模块 — USART3, 115200 8N1
 *
 * 接线: ESP-01S TX → PB11 (USART3_RX), ESP-01S RX → PB10 (USART3_TX)
 * 协议: 文本行, '\n' 结尾
 *   STM32→ESP: STATUS,<state>,<ldr>,<ntc>\n
 *   ESP→STM32: CMD,<cmd>\n
 */
#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"

void ESP8266_Init(void);

/** 发送一行文本到 ESP-01S (自动加 \n) */
void ESP8266_Send(const char *fmt, ...);

/** 从 USART3 读一个字节 (非阻塞), -1=无数据 */
int ESP8266_GetChar(void);

/** 供 USART3_IRQHandler 调用: 把中断收到的字节存入环形缓冲区 */
void ESP8266_RxIRQPush(uint8_t byte);

/** 解析收到的字符, 返回解析出的命令类型, CMD_NONE=无 */
#include "pet_fsm.h"  /* PetCmd_t */
PetCmd_t ESP8266_Process(void);

#endif
