/**
 * @file    w25q_spi.h
 * @brief   W25Q 系列 SPI Flash 驱动
 *
 * SPI1: PA5=SCK, PA6=MISO, PA7=MOSI, PA4=CS
 */
#ifndef __W25Q_SPI_H
#define __W25Q_SPI_H

#include "main.h"

/* W25Q 基本信息 */
#define W25Q_PAGE_SIZE      256     /* 页大小: 256 字节 */
#define W25Q_SECTOR_SIZE    4096    /* 扇区: 4KB */
#define W25Q_BLOCK_SIZE     65536   /* 块: 64KB */

/** 初始化 SPI + 读取芯片 ID */
uint8_t W25Q_Init(void);           /* 返回 1=成功, 0=失败 */

/** 读取数据 */
void W25Q_Read(uint32_t addr, uint8_t *buf, uint32_t len);

/** 页写入 (不超过256字节, 不跨页) */
void W25Q_WritePage(uint32_t addr, uint8_t *buf, uint16_t len);

/** 扇区擦除 (4KB) */
void W25Q_EraseSector(uint32_t addr);

/** 芯片全擦除 (耗时较长, ~10s) */
void W25Q_EraseChip(void);

/** 读取 JEDEC ID (3字节: 制造商+类型+容量) */
void W25Q_ReadID(uint8_t *id);

#endif
