/**
 * @file    w25q_spi.c
 * @brief   W25Q SPI Flash 驱动实现
 */
#include "w25q_spi.h"
#include "spi.h"
#include "gpio.h"
#include <stdio.h>

/* W25Q 命令 */
#define CMD_WRITE_ENABLE  0x06
#define CMD_READ_STATUS   0x05
#define CMD_READ_DATA     0x03
#define CMD_PAGE_PROGRAM  0x02
#define CMD_SECTOR_ERASE  0x20
#define CMD_CHIP_ERASE    0xC7
#define CMD_JEDEC_ID      0x9F

/* PA4 = CS */
#define W25Q_CS_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define W25Q_CS_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

static uint8_t g_w25q_ok = 0;

/* ---- SPI 基础收发 ---- */

static uint8_t spi_txrx(uint8_t tx)
{
    uint8_t rx;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 100);
    return rx;
}

static void spi_tx(uint8_t tx)
{
    HAL_SPI_Transmit(&hspi1, &tx, 1, 100);
}

/* ---- W25Q 操作 ---- */

static void w25q_write_enable(void)
{
    W25Q_CS_LOW();
    spi_tx(CMD_WRITE_ENABLE);
    W25Q_CS_HIGH();
}

static uint8_t w25q_read_status(void)
{
    uint8_t st;
    W25Q_CS_LOW();
    spi_tx(CMD_READ_STATUS);
    st = spi_txrx(0xFF);
    W25Q_CS_HIGH();
    return st;
}

static void w25q_wait_busy(void)
{
    while (w25q_read_status() & 0x01);  /* 等 BUSY 位清零 */
}

/* ---- API ---- */

void W25Q_ReadID(uint8_t *id)
{
    W25Q_CS_LOW();
    spi_tx(CMD_JEDEC_ID);
    id[0] = spi_txrx(0xFF);  /* Manufacturer */
    id[1] = spi_txrx(0xFF);  /* Memory Type */
    id[2] = spi_txrx(0xFF);  /* Capacity */
    W25Q_CS_HIGH();
}

uint8_t W25Q_Init(void)
{
    /* CS 初始化为高 */
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_4;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &g);
    W25Q_CS_HIGH();

    /* 读 ID 验证芯片 */
    uint8_t id[3];
    W25Q_ReadID(id);

    /* 常见 ID: EF 40 18 (W25Q64), EF 40 17 (W25Q32), EF 40 16 (W25Q16) */
    if (id[0] == 0xEF || id[0] == 0xC8 || id[0] == 0x1F) {
        g_w25q_ok = 1;
        printf("[W25Q] ID: %02X %02X %02X - OK\r\n", id[0], id[1], id[2]);
        return 1;
    } else {
        g_w25q_ok = 0;
        printf("[W25Q] ID: %02X %02X %02X - FAILED (expected EF/1F/C8)\r\n",
               id[0], id[1], id[2]);
        return 0;
    }
}

void W25Q_Read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if (!g_w25q_ok) return;

    W25Q_CS_LOW();
    spi_tx(CMD_READ_DATA);
    spi_tx((addr >> 16) & 0xFF);  /* A23-A16 */
    spi_tx((addr >> 8)  & 0xFF);  /* A15-A8  */
    spi_tx(addr & 0xFF);           /* A7-A0   */
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = spi_txrx(0xFF);
    }
    W25Q_CS_HIGH();
}

void W25Q_WritePage(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (!g_w25q_ok) return;
    if (len > W25Q_PAGE_SIZE) len = W25Q_PAGE_SIZE;

    w25q_write_enable();
    W25Q_CS_LOW();
    spi_tx(CMD_PAGE_PROGRAM);
    spi_tx((addr >> 16) & 0xFF);
    spi_tx((addr >> 8)  & 0xFF);
    spi_tx(addr & 0xFF);
    for (uint16_t i = 0; i < len; i++) {
        spi_tx(buf[i]);
    }
    W25Q_CS_HIGH();
    w25q_wait_busy();
}

void W25Q_EraseSector(uint32_t addr)
{
    if (!g_w25q_ok) return;

    w25q_write_enable();
    W25Q_CS_LOW();
    spi_tx(CMD_SECTOR_ERASE);
    spi_tx((addr >> 16) & 0xFF);
    spi_tx((addr >> 8)  & 0xFF);
    spi_tx(addr & 0xFF);
    W25Q_CS_HIGH();
    w25q_wait_busy();
}

void W25Q_EraseChip(void)
{
    if (!g_w25q_ok) return;

    printf("[W25Q] Chip erase... (may take ~10s)\r\n");
    w25q_write_enable();
    W25Q_CS_LOW();
    spi_tx(CMD_CHIP_ERASE);
    W25Q_CS_HIGH();
    w25q_wait_busy();  /* 可能需要 10-20 秒 */
    printf("[W25Q] Chip erase done\r\n");
}
