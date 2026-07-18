/**
 * @file    hc05.c
 * @brief   HC-05 蓝牙模块 — 单字符命令 (9600 8N1)
 *
 * 所有命令均为单字符, 轮询接收:
 *   t=摸头  f=喂食  z=睡觉  s=状态
 *   1=查询温度  2=查询温度+亮度
 *   3/4=调高/低调高温阈值(±50)  5/6=调高/低调暗光阈值(±50)
 *   h=帮助
 */
#include "hc05.h"
#include "usart.h"
#include "sensor.h"
#include "buzzer.h"
#include "flash_config.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

void HC05_Init(void)
{
    MX_USART2_UART_Init();
    printf("[HC-05] Bluetooth ready on USART2 (9600 8N1)\r\n");
    HAL_Delay(500);
    HC05_Printf("MoonCat v2.0 Ready");
    printf("[HC-05] Boot message sent\r\n");
}

int HC05_GetChar(void)
{
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
        return (uint8_t)(huart2.Instance->DR & 0xFF);
    }
    return -1;
}

/* 通过 HC-05 发送文本 (手机端可见) */
void HC05_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsprintf(buf, fmt, args);
    va_end(args);

    if (len <= 0) return;

    static const char crlf[] = "\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, len, 100);
    HAL_UART_Transmit(&huart2, (uint8_t *)crlf, 2, 100);
}

/* 解析收到的字符, 返回宠物指令 */
PetCmd_t HC05_Process(void)
{
    PetCmd_t result = CMD_NONE;
    int ch = HC05_GetChar();
    if (ch < 0) return CMD_NONE;

    switch (ch) {
        /* 宠物交互 */
        case 't': case 'T': result = CMD_TOUCH; Buzzer_Beep(30); printf("[BT-TOUCH]\r\n"); break;
        case 'f': case 'F': result = CMD_FEED;  Buzzer_Beep(30); printf("[BT-FEED]\r\n");  break;
        case 'z': case 'Z': result = CMD_SLEEP; Buzzer_Beep(30); printf("[BT-SLEEP]\r\n"); break;
        /* 状态查询 */
        case 's': case 'S': {
            const char *sn[] = {"IDLE","HAPPY","SLEEP","SURPRISE","HOT"};
            HC05_Printf("State=%s LDR=%d NTC=%d hot=%d dark=%d",
                        sn[PetFSM_GetState()],
                        Sensor_GetLDR(), Sensor_GetNTC(),
                        g_ntc_hot, g_ldr_dark);
            printf("[BT-STATUS]\r\n");
            break;
        }
        /* 传感器查询 */
        case '1':
            HC05_Printf("TEMP=%d", Sensor_GetNTC());
            printf("[BT] TEMP=%d\r\n", Sensor_GetNTC());
            break;
        case '2':
            HC05_Printf("TEMP=%d LIGHT=%d", Sensor_GetNTC(), Sensor_GetLDR());
            printf("[BT] T=%d L=%d\r\n", Sensor_GetNTC(), Sensor_GetLDR());
            break;
        /* 阈值调节 */
        case '3': g_ntc_hot += 50; HC05_Printf("hot=%d", g_ntc_hot); printf("[BT] hot=%d\r\n", g_ntc_hot); break;
        case '4': if (g_ntc_hot >= 100) g_ntc_hot -= 50; HC05_Printf("hot=%d", g_ntc_hot); printf("[BT] hot=%d\r\n", g_ntc_hot); break;
        case '5': g_ldr_dark += 50; HC05_Printf("dark=%d", g_ldr_dark); printf("[BT] dark=%d\r\n", g_ldr_dark); break;
        case '6': if (g_ldr_dark >= 100) g_ldr_dark -= 50; HC05_Printf("dark=%d", g_ldr_dark); printf("[BT] dark=%d\r\n", g_ldr_dark); break;
        /* 保存设置到 Flash */
        case 'w': case 'W':
            Buzzer_Beep(30);
            FlashConfig_Save();
            HC05_Printf("OK saved");
            printf("[BT] saved: hot=%d dark=%d\r\n", g_ntc_hot, g_ldr_dark);
            break;
        /* 帮助 */
        case 'h': case 'H':
            HC05_Printf("t/f/z=pet 1=temp 2=all 3/4=hot+- 5/6=dark+- s=status");
            break;
        default: break;
    }

    return result;
}
