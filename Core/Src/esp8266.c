/**
 * @file    esp8266.c
 * @brief   ESP-01S WiFi 模块驱动 — USART3 文本协议
 *
 * 使用 PB10(TX)/PB11(RX). RX 采用中断+环形缓冲区接收,
 * 避免 115200bps 突发数据在主循环阻塞期间 (如OLED渲染) 被覆盖丢失.
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

/* 中断接收环形缓冲区 (ISR只写, 主循环只读, 单生产者单消费者无需加锁) */
#define ESP_RXBUF_SIZE 128
static volatile uint8_t  g_irq_buf[ESP_RXBUF_SIZE];
static volatile uint16_t g_irq_head = 0;  /* ISR 写入位置 */
static volatile uint16_t g_irq_tail = 0;  /* 主循环读取位置 */

void ESP8266_RxIRQPush(uint8_t byte)
{
    uint16_t next = (g_irq_head + 1) % ESP_RXBUF_SIZE;
    if (next != g_irq_tail) {  /* 缓冲区未满才写入, 满了就丢弃最新字节 */
        g_irq_buf[g_irq_head] = byte;
        g_irq_head = next;
    }
}

void ESP8266_Init(void)
{
    MX_USART3_UART_Init();
    g_rx_idx = 0;
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    printf("[ESP8266] USART3 ready (115200 8N1, IRQ RX)\r\n");
}

int ESP8266_GetChar(void)
{
    if (g_irq_tail == g_irq_head) return -1;  /* 缓冲区空 */
    uint8_t ch = g_irq_buf[g_irq_tail];
    g_irq_tail = (g_irq_tail + 1) % ESP_RXBUF_SIZE;
    return ch;
}

void ESP8266_Send(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsprintf(buf, fmt, args);
    va_end(args);

    if (len <= 0 || (size_t)len >= sizeof(buf)) return;

    /* 追加 \n 换行 */
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
