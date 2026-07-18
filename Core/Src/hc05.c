/**
 * @file    hc05.c
 * @brief   HC-05 蓝牙模块 — USART2 透传 (9600 8N1)
 *
 * 使用 PA2(TX)/PA3(RX), 与 USART1 相同的轮询方式读取.
 * 上电后 HC-05 默认处于透传模式, 手机蓝牙串口助手连接后即可通信.
 */
#include "hc05.h"
#include "usart.h"
#include <stdio.h>

void HC05_Init(void)
{
    MX_USART2_UART_Init();
    printf("[HC-05] Bluetooth ready on USART2 (9600 8N1)\r\n");
}

int HC05_GetChar(void)
{
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
        return (uint8_t)(huart2.Instance->DR & 0xFF);
    }
    return -1;
}
