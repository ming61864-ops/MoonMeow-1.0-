/**
 * @file    flash_config.c
 * @brief   Flash 配置存储 - W25Q 扇区0
 */
#include "flash_config.h"
#include "w25q_spi.h"
#include "sensor.h"
#include <stdio.h>
#include <string.h>

#define CONFIG_MAGIC  0xCA7CA700

/* packed 防止编译器对齐填充导致 sizeof 不一致 */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t ldr_dark;
    uint16_t ldr_bright;
    uint16_t ntc_hot;
    uint16_t ntc_cold;
    uint8_t  crc;        /* 简单 XOR 校验 */
    uint8_t  _pad;       /* 填充到偶数 */
} FlashConfig_t;

static uint8_t calc_crc(FlashConfig_t *cfg)
{
    uint8_t *p = (uint8_t *)cfg;
    uint8_t crc = 0;
    /* 校验 magic + 4个阈值 = 4+2*4 = 12 字节 */
    for (int i = 0; i < 12; i++) {
        crc ^= p[i];
    }
    return crc;
}

void FlashConfig_Load(void)
{
    FlashConfig_t cfg;

    W25Q_Read(CONFIG_FLASH_ADDR, (uint8_t *)&cfg, sizeof(cfg));

    if (cfg.magic != CONFIG_MAGIC) {
        printf("[CONFIG] No saved config, using defaults\r\n");
        return;
    }

    uint8_t expected = calc_crc(&cfg);
    if (cfg.crc != expected) {
        printf("[CONFIG] CRC mismatch (got 0x%02X expected 0x%02X), using defaults\r\n",
               cfg.crc, expected);
        return;
    }

    g_ldr_dark   = cfg.ldr_dark;
    g_ldr_bright = cfg.ldr_bright;
    g_ntc_hot    = cfg.ntc_hot;
    g_ntc_cold   = cfg.ntc_cold;

    printf("[CONFIG] Loaded: LDR=%d/%d NTC=%d/%d\r\n",
           g_ldr_dark, g_ldr_bright, g_ntc_hot, g_ntc_cold);
}

void FlashConfig_Save(void)
{
    FlashConfig_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.magic      = CONFIG_MAGIC;
    cfg.ldr_dark   = g_ldr_dark;
    cfg.ldr_bright = g_ldr_bright;
    cfg.ntc_hot    = g_ntc_hot;
    cfg.ntc_cold   = g_ntc_cold;
    cfg.crc        = calc_crc(&cfg);

    W25Q_EraseSector(CONFIG_FLASH_ADDR);
    W25Q_WritePage(CONFIG_FLASH_ADDR, (uint8_t *)&cfg, sizeof(cfg));

    printf("[CONFIG] Saved: LDR=%d/%d NTC=%d/%d (CRC=0x%02X size=%d)\r\n",
           g_ldr_dark, g_ldr_bright, g_ntc_hot, g_ntc_cold,
           cfg.crc, (int)sizeof(cfg));
}

void FlashConfig_Reset(void)
{
    g_ldr_dark   = 3000;
    g_ldr_bright = 1200;
    g_ntc_hot    = 1300;
    g_ntc_cold   = 2800;

    W25Q_EraseSector(CONFIG_FLASH_ADDR);

    printf("[CONFIG] Reset to defaults\r\n");
}
