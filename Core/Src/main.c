/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "ssd1306_i2c.h"
#include "sensor.h"
#include "buzzer.h"
#include "w25q_spi.h"
#include "gfx_engine.h"
#include "pet_fsm.h"
#include "cat_fullscreen.h"
#include "cat_anim1.h"
#include "cat_anim2.h"
#include "cat_sleep.h"
#include "cat_hot.h"
#include "flash_config.h"
#include "hc05.h"
#include "esp8266.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* printf 重定向到 USART1 (Keil MicroLIB 使用 fputc) */
#ifdef __MICROLIB
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 10);
    return ch;
}
#else
/* GCC/ARM Compiler 使用 _write */
int _write(int fd, char *ptr, int len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 10);
    return len;
}
#endif

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("================================\r\n");
  printf("  STM32 MoonCat v2.0\r\n");
  printf("  MCU: STM32F103C8T6 @ 72MHz\r\n");
  printf("================================\r\n\n");

  /* Init all hardware */
  SSD1306_Init();
  W25Q_Init();
  FlashConfig_Load();  /* 必须在 Sensor_Init 之前, 加载已保存阈值 */
  Sensor_Init();
  Buzzer_Init();
  HC05_Init();
  ESP8266_Init();
  PetFSM_Init();

  /* Boot screen */
  GFX_Clear();
  GFX_DrawString(20, 0, "MoonCat v2.0");
  GFX_DrawLine(0, 10, 127, 10);
  GFX_DrawString(0, 16, "OLED  SENS  BUZZ");
  GFX_DrawString(0, 26, "FLASH FSM   READY");
  GFX_DrawString(30, 45, "Booting...");
  GFX_Refresh();
  HAL_Delay(1200);

  Buzzer_Beep(100);
  printf("\n=== System Boot OK! ===\r\n");
  printf("Commands: touch(t) feed(f) status(s) help(h)\r\n\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    static uint32_t last_sensor = 0;
    static uint32_t last_led = 0;
    static uint32_t last_status = 0;
    static uint32_t last_report = 0;
    uint32_t now = HAL_GetTick();

    /* ---- ESP-01S WiFi 状态上报 (5s) ---- */
    if (now - last_report >= 5000) {
        last_report = now;
        const char *sn[] = {"IDLE","HAPPY","SLEEP","SURPRISE","HOT"};
        printf("[ESP-SEND] tick=%lu\r\n", (unsigned long)now);  /* 诊断: 确认STM32实际发送节奏 */
        ESP8266_Send("STATUS,%s,%d,%d",
                     sn[PetFSM_GetState()],
                     Sensor_GetLDR(),
                     Sensor_GetNTC());
    }

    /* ---- 串口命令接收 (单字符即时触发, 不需要回车) ---- */
    static PetCmd_t pet_cmd = CMD_NONE;
    while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE)) {
        uint8_t ch = (uint8_t)(huart1.Instance->DR & 0xFF);

        switch (ch) {
            case 't': case 'T':
                pet_cmd = CMD_TOUCH;
                Buzzer_Beep(30);
                printf("[TOUCH]\r\n");
                break;
            case 'f': case 'F':
                pet_cmd = CMD_FEED;
                Buzzer_Beep(30);
                printf("[FEED]\r\n");
                break;
            case 's': case 'S':
                last_status = 0;
                break;
            case 'z': case 'Z':
                pet_cmd = CMD_SLEEP;
                Buzzer_Beep(30);
                printf("[SLEEP]\r\n");
                break;
            case 'w': case 'W':
                FlashConfig_Save();
                Buzzer_Beep(30);
                break;
            case 'r': case 'R':
                FlashConfig_Reset();
                Buzzer_Beep(100);
                printf("[RESET] defaults restored\r\n");
                break;
            case '=':
                g_ntc_hot += 50;
                printf("[NTC] hot threshold: %d\r\n", g_ntc_hot);
                break;
            case '-':
                if (g_ntc_hot >= 100) g_ntc_hot -= 50;
                printf("[NTC] hot threshold: %d\r\n", g_ntc_hot);
                break;
            case 'h': case 'H':
                printf("\r\n=== CAT PET COMMANDS ===\r\n");
                printf("  t - touch   f - feed\r\n");
                printf("  z - sleep   s - status\r\n");
                printf("  w - save    r - reset\r\n");
                printf("  =/- adj NTC hot\r\n");
                printf("========================\r\n\n");
                break;
            default:
                break;  /* 忽略 \r \n 等 */
        }
    }

    /* ---- HC-05 蓝牙命令 ---- */
    {
        PetCmd_t bt_cmd = HC05_Process();
        if (bt_cmd != CMD_NONE) {
            pet_cmd = bt_cmd;
        }
    }

    /* ---- ESP-01S WiFi 云端指令 ---- */
    {
        PetCmd_t cloud_cmd = ESP8266_Process();
        if (cloud_cmd != CMD_NONE) {
            pet_cmd = cloud_cmd;
        }
    }

    /* ---- 传感器 (500ms) ---- */
    if (now - last_sensor >= 500) {
        last_sensor = now;
        Sensor_Update();
    }

    /* ---- 状态机更新 (200ms) ---- */
    static uint32_t last_fsm = 0;
    if (now - last_fsm >= 200) {
        last_fsm = now;
        PetFSM_Update(Sensor_GetLight(), Sensor_GetTemp(), pet_cmd);
        pet_cmd = CMD_NONE;  /* 命令已消费, 重置 */

        /* 触发音效 */
        int8_t mel = PetFSM_GetMelody();
        if (mel >= 0) Buzzer_PlayMelody((Melody_t)mel);
    }

    /* ---- 渲染 (帧速按情绪可调) ---- */
    static uint32_t last_render = 0;
    static uint8_t  cat_frame = 0;
    static uint8_t  last_anim_slot = 99;

    /* 不同状态不同帧速 */
    uint16_t frame_ms = 150;  /* IDLE 默认 */
    switch (PetFSM_GetAnimSlot()) {
        case ANIM_IDLE:    frame_ms = 150; break;
        case ANIM_HAPPY:   frame_ms = 100; break;
        case ANIM_SLEEP:   frame_ms = 300; break;
        case ANIM_SURPRISE: frame_ms = 80;  break;
        case ANIM_HOT:      frame_ms = 80;  break;
    }

    if (now - last_render >= frame_ms) {
        last_render = now;

        const uint8_t *full = NULL;
        uint8_t max_frames = 6;

        /* 换动画时重置帧计数 */
        if (PetFSM_GetAnimSlot() != last_anim_slot) {
            cat_frame = 0;
            last_anim_slot = PetFSM_GetAnimSlot();
        }

        switch (PetFSM_GetAnimSlot()) {
            case ANIM_IDLE:
                full = cat_idle_frames[cat_frame];
                max_frames = CAT_IDLE_FRAMES;
                break;
            case ANIM_HAPPY:
                full = cat_anim2_frames[cat_frame];
                max_frames = CAT_ANIM2_FRAMES;
                break;
            case ANIM_SLEEP:
                full = cat_sleep_frames[cat_frame];
                max_frames = CAT_SLEEP_FRAMES;
                break;
            case ANIM_SURPRISE:
                full = cat_anim1_frames[cat_frame];
                max_frames = CAT_ANIM1_FRAMES;
                break;
            case ANIM_HOT:
                full = cat_hot_frames[cat_frame];
                max_frames = CAT_HOT_FRAMES;
                break;
        }

        /* 直接写OLED (与参考代码完全一致的逐页逐列方式) */
        if (full) {
            for (uint8_t page = 0; page < 8; page++) {
                SSD1306_WriteCmd(0xB0 + page);
                SSD1306_WriteCmd(0x00);
                SSD1306_WriteCmd(0x10);
                for (uint8_t col = 0; col < 128; col++) {
                    SSD1306_WriteData(full[page * 128 + col]);
                }
            }
        }

        /* 下一帧 */
        if (max_frames > 0) {
            cat_frame = (cat_frame + 1) % max_frames;
        }
    }

    /* ---- LED 状态指示 (不同情绪不同闪烁频率) ---- */
    {
        uint16_t led_ms = 500;  /* IDLE: 1Hz */
        switch (PetFSM_GetAnimSlot()) {
            case ANIM_IDLE:    led_ms = 500;  break;  /* 慢闪 */
            case ANIM_HAPPY:   led_ms = 200;  break;  /* 快闪 */
            case ANIM_SLEEP:   led_ms = 1500; break;  /* 很慢 */
            case ANIM_SURPRISE: led_ms = 100;  break;  /* 极快 */
            case ANIM_HOT:      led_ms = 80;   break;  /* 最快: 过热警告 */
        }
        if (now - last_led >= led_ms) {
            last_led = now;
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }

    /* ---- 串口状态输出 (5s) ---- */
    if (now - last_status >= 5000) {
        last_status = now;
        const char *sn[] = {"IDLE","HAPPY","SLEEP","SURPRISE","HOT"};
        const char *an[] = {"idle","happy","sleep","surprise","hot"};
        printf("[CAT] State=%s Anim=%s LDR=%d NTC=%d\r\n",
               sn[PetFSM_GetState()],
               an[PetFSM_GetAnimSlot()],
               Sensor_GetLDR(), Sensor_GetNTC());
    }

    /* ---- 蜂鸣器 ---- */
    Buzzer_Update();

    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
