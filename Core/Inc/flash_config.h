/**
 * @file    flash_config.h
 * @brief   W25Q Flash 配置存储 - 传感器阈值掉电保存
 */
#ifndef __FLASH_CONFIG_H
#define __FLASH_CONFIG_H

#include "main.h"

/* Flash 存储地址 (使用最后一个扇区, 不干扰其他数据) */
#define CONFIG_FLASH_ADDR  0x000000

void FlashConfig_Load(void);   /* 上电加载, 失败则用默认值 */
void FlashConfig_Save(void);   /* 保存当前阈值到 Flash */
void FlashConfig_Reset(void);  /* 恢复默认阈值 + 擦除 Flash 配置 */

#endif
