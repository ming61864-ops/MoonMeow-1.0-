/**
 * @file    esp8266.c
 * @brief   ESP-01S WiFi 模块驱动 — USART3 文本协议
 *
 * 使用 PB10(TX)/PB11(RX), 与 USART1/USART2 相同的轮询方式.
 * 接收侧用行缓冲, 遇到 \n 解析指令.
 */
#include "esp8266.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* 行缓冲 */
static char  g_rx_buf[64];
static uint8_t g_rx_idx = 0;

void ESP8266_Init(void)
{
    MX_USART3_UART_Init();
    g_rx_idx = 0;
    printf("[ESP8266] USART3 ready (115200 8N1)\r\n");
}

int ESP8266_GetChar(void)
{
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE)) {
        return (uint8_t)(huart3.Instance->DR & 0xFF);
    }
    return -1;
}

void ESP8266_Send(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len <= 0) return;
    if ((size_t)len >= sizeof(buf)) len = sizeof(buf) - 1;

    /* 追加 \n 换行 */
    if ((size_t)len < sizeof(buf) - 1)
        buf[len++] = '\n';

    HAL_UART_Transmit(&huart3, (uint8_t *)buf, len, 100);
}

PetCmd_t ESP8266_Process(void)
{
    PetCmd_t result = CMD_NONE;

    while (1) {
        int ch = ESP8266_GetChar();
        if (ch < 0) break;

        /* 忽略 \r */
        if (ch == '\r') continue;

        /* 行结束 */
        if (ch == '\n') {
            g_rx_buf[g_rx_idx] = '\0';
            g_rx_idx = 0;

            /* 解析 CMD,<name> */
            if (strncmp(g_rx_buf, "CMD,", 4) == 0) {
                const char *cmd = g_rx_buf + 4;
                if (strcmp(cmd, "TOUCH") == 0) {
                    printf("[ESP] Cloud: TOUCH\r\n");
                    result = CMD_TOUCH;
                } else if (strcmp(cmd, "FEED") == 0) {
                    printf("[ESP] Cloud: FEED\r\n");
                    result = CMD_FEED;
                } else if (strcmp(cmd, "SLEEP") == 0) {
                    printf("[ESP] Cloud: SLEEP\r\n");
                    result = CMD_SLEEP;
                }
            }
            /* 回显其他行到串口 (调试用) */
            else if (g_rx_buf[0] != '\0') {
                printf("[ESP] %s\r\n", g_rx_buf);
            }
            continue;
        }

        /* 普通字符加入缓冲 */
        if (g_rx_idx < sizeof(g_rx_buf) - 1) {
            g_rx_buf[g_rx_idx++] = (char)ch;
        }
    }

    return result;
}
