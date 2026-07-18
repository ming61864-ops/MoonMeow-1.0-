/**
 * @file    pet_fsm.c
 * @brief   月薪喵情绪状态机 - 5状态版本
 *
 * 状态转换规则:
 *   过热          → HOT (即时)
 *   暗光持续5s   → SLEEP
 *   CMD_TOUCH     → SURPRISE (持续5秒后恢复)
 *   CMD_FEED      → HAPPY (持续5秒后恢复)
 *   其他          → IDLE (默认)
 */

#include "pet_fsm.h"
#include "sensor.h"
#include "buzzer.h"

/* 最小状态停留时间 (ms), 防抖动 */
#define MIN_DWELL    5000

/* 命令持续时长 */
#define SURPRISE_DUR 5000
#define FEED_DUR     5000

/* 睡眠触发: 暗光持续 5 秒 */
#define SLEEP_TIMEOUT 5000

static PetState_t g_state = STATE_IDLE;
static PetState_t g_prev_state = STATE_IDLE;  /* 命令打断前的状态 */
static uint32_t   g_state_enter = 0;
static uint32_t   g_cmd_end = 0;
static uint8_t    g_cmd_active = 0;
static uint32_t   g_dark_start = 0;
static uint8_t    g_anim_slot = ANIM_IDLE;
static int8_t     g_melody = -1;  /* -1=无 */

/* ==================== 实现 ==================== */

void PetFSM_Init(void)
{
    g_state = STATE_IDLE;
    g_state_enter = HAL_GetTick();
    g_anim_slot = ANIM_IDLE;
}

void PetFSM_Update(uint8_t light, uint8_t temp, PetCmd_t cmd)
{
    uint32_t now = HAL_GetTick();

    /* ---- 处理命令 ---- */
    if (cmd == CMD_TOUCH && !g_cmd_active) {
        g_prev_state = g_state;
        g_state = STATE_SURPRISE;
        g_state_enter = now;
        g_cmd_end = now + SURPRISE_DUR;
        g_cmd_active = 1;
    }
    if (cmd == CMD_FEED && !g_cmd_active) {
        g_prev_state = g_state;
        g_state = STATE_HAPPY;
        g_state_enter = now;
        g_cmd_end = now + FEED_DUR;
        g_cmd_active = 1;
    }
    if (cmd == CMD_SLEEP && !g_cmd_active) {
        g_prev_state = g_state;
        g_state = STATE_SLEEP;
        g_state_enter = now;
        g_cmd_end = now + FEED_DUR;
        g_cmd_active = 1;
    }

    /* 命令到期 → 恢复 */
    if (g_cmd_active && now >= g_cmd_end) {
        g_state = g_prev_state;
        g_state_enter = now;
        g_cmd_active = 0;
    }

    /* 命令激活期间不允许环境切换 */
    if (g_cmd_active) goto apply_state;

    /* 最小停留时间 */
    if (now - g_state_enter < MIN_DWELL) goto apply_state;

    /* ---- 环境 → 状态 ---- */

    /* 过热优先: 温度过高 → HOT */
    if (temp == TEMP_HOT && g_state != STATE_SLEEP) {
        if (g_state != STATE_HOT) {
            g_state = STATE_HOT;
            g_state_enter = now;
        }
    }
    /* 降温 → 回到 IDLE */
    if (temp != TEMP_HOT && g_state == STATE_HOT) {
        g_state = STATE_IDLE;
        g_state_enter = now;
    }

    /* 暗光触发睡眠 (HOT状态下不睡眠) */
    if (light == LIGHT_DARK && g_state != STATE_HOT) {
        /* 暗光 → 计时, 超5秒 → SLEEP */
        if (g_dark_start == 0) g_dark_start = now;
        if (now - g_dark_start >= SLEEP_TIMEOUT && g_state != STATE_SLEEP) {
            g_state = STATE_SLEEP;
            g_state_enter = now;
        }
    } else {
        g_dark_start = 0;
        /* 非暗光且不是睡眠状态 → IDLE */
        if (g_state == STATE_SLEEP) {
            g_state = STATE_IDLE;
            g_state_enter = now;
        }
    }

    /* 非HOT、非睡眠、非暗光 → IDLE */
    if (light != LIGHT_DARK && g_state != STATE_SLEEP && g_state != STATE_HOT && !g_cmd_active) {
        if (g_state != STATE_IDLE && now - g_state_enter >= MIN_DWELL) {
            g_state = STATE_IDLE;
            g_state_enter = now;
        }
    }

apply_state:
    /* ---- 根据状态设置动画和音效 ---- */
    g_melody = -1;

    switch (g_state) {
        case STATE_IDLE:
            g_anim_slot = ANIM_IDLE;
            break;
        case STATE_HAPPY:
            g_anim_slot = ANIM_HAPPY;
            if (now - g_state_enter < 500) g_melody = MELODY_HAPPY;
            break;
        case STATE_SLEEP:
            g_anim_slot = ANIM_SLEEP;
            if (now - g_state_enter < 500) g_melody = MELODY_CLICK;
            break;
        case STATE_SURPRISE:
            g_anim_slot = ANIM_SURPRISE;
            if (now - g_state_enter < 500) g_melody = MELODY_SURPRISE;
            break;
        case STATE_HOT:
            g_anim_slot = ANIM_HOT;
            if (now - g_state_enter < 500) g_melody = MELODY_ANNOYED;
            break;
    }
}

PetState_t PetFSM_GetState(void)   { return g_state; }
uint8_t    PetFSM_GetAnimSlot(void) { return g_anim_slot; }
int8_t     PetFSM_GetMelody(void)   { return g_melody; }
