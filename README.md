# 月薪喵 MoonCat v2.0

> 基于 STM32F103C8T6 的环境感知虚拟桌面宠物 · 128×64 OLED 全屏动画 · 5 状态情绪 FSM · HC-05 蓝牙控制 · ESP-01S WiFi/MQTT 远程联动

月薪喵是一只跑在 0.96 寸 OLED 屏上的裸机嵌入式桌面宠物。它通过光敏、热敏传感器感知环境变化，自动在 5 种情绪状态间切换。支持三种交互方式：**CH340 串口**（调试+配置）、**HC-05 蓝牙**（手机/手柄远程控制）、**ESP-01S WiFi**（MQTT 云端远程控制）。传感器阈值保存在 SPI Flash 中，掉电不丢失。

---

## 硬件平台

| 模块 | 接口 | 引脚 | 说明 |
|------|------|------|------|
| STM32F103C8T6 | — | — | Cortex-M3 内核，72 MHz |
| 0.96 寸 SSD1306 OLED | I²C（400 kHz）| PB6 SCL / PB7 SDA | 128×64 白色像素 |
| W25Q 8 Mbit SPI Flash | SPI（18 MHz）| PA4 CS / PA5 SCK / PA6 MISO / PA7 MOSI | 配置持久化 |
| 光敏电阻（LDR）| ADC1 CH0 | PA0 | 环境光照检测 |
| 热敏电阻（NTC）| ADC1 CH1 | PA1 | 环境温度检测 |
| 有源蜂鸣器 | GPIO | PB0 | LOW=响，HIGH=静音 |
| CH340 USB 转 TTL | USART1 115200 | PA9 TX / PA10 RX | 调试日志 + 串口命令 |
| **HC-05 蓝牙** | **USART2 9600** | **PA2 TX / PA3 RX** | 手机遥控 + 传感器查询 |
| **ESP-01S WiFi** | **USART3 115200（中断接收）** | **PB10 TX / PB11 RX** | MQTT 云端远程控制 |
| 板载 LED | GPIO | PC13 | 情绪闪烁指示 |

---

## 五种情绪状态

| 状态 | 触发条件 | 动画表现 | 帧率 |
|------|---------|---------|------|
| **😐 待机 IDLE** | 正常光照 + 舒适温度 | 眨眼、偶尔抖耳朵 | ~7 fps |
| **😊 开心 HAPPY** | 明亮 + 舒适，或收到 `f` 喂食指令 | 摇尾巴、蹦跳 | 10 fps |
| **😴 睡觉 SLEEP** | 暗光环境持续 5 秒以上 | 闭眼、呼吸起伏、飘 Zzz… | ~3 fps |
| **😲 惊讶 SURPRISE** | 收到 `t` 摸头指令 | 竖耳朵、惊讶脸 | ~12 fps |
| **🥵 烦躁 HOT** | 温度超阈值或环境过亮 | 皱眉、喘气、抖腿 | ~12 fps |

---

## 交互方式

### 1. CH340 串口（调试 + 配置）

| 按键 | 功能 |
|------|------|
| `t` | 摸头 → SURPRISE |
| `f` | 喂食 → HAPPY |
| `z` | 强制 SLEEP |
| `s` | 查看状态 |
| `=` / `-` | 调节 NTC 热阈值 ±50 |
| `w` | 保存阈值到 Flash |
| `r` | 恢复默认阈值 |
| `h` | 帮助菜单 |

### 2. HC-05 蓝牙（手机遥控）

| 按键 | 功能 | 手机回复 |
|------|------|---------|
| `t` | 摸头 | — |
| `f` | 喂食 | — |
| `z` | 睡觉 | — |
| `s` | 查询状态 | `State=IDLE LDR=450 NTC=820 hot=1200 dark=800` |
| `1` | 查询温度 | `TEMP=820` |
| `2` | 查询温度+亮度 | `TEMP=820 LIGHT=450` |
| `3` / `4` | 高温阈值 ±50 | `hot=1200` |
| `5` / `6` | 暗光阈值 ±50 | `dark=800` |
| `w` | 保存到 Flash | `OK saved` |
| `h` | 帮助 | 命令列表 |

### 3. ESP-01S WiFi（MQTT 远程控制）

STM32 通过 USART3 与 ESP-01S 桥接，ESP-01S 连接 WiFi 后接入 MQTT broker（默认 `broker.emqx.io`，测试用公共服务器）。

| 方向 | Topic | 内容 |
|------|-------|------|
| STM32 → 云端 | `winner_mooncat/status` | `{"mood":"...","light":...,"temp":...}`，每 5 秒上报一次 |
| 云端 → STM32 | `winner_mooncat/cmd` | 发送 `TOUCH` / `FEED` / `SLEEP` 纯文本 |

用 MQTTX 等客户端连接 broker，订阅 `status` topic 查看宠物实时状态，向 `cmd` topic 发送指令远程控制宠物。

```
Tools/esp8266_mqtt/esp8266_mqtt.ino   # ESP-01S 固件（Arduino IDE 烧录）
```

烧录前需在 `.ino` 顶部修改 `WIFI_SSID` / `WIFI_PASS` 为实际 WiFi 信息。

---

## 固件架构

```
应用层:      main.c（主循环调度 ~100 Hz）
中间件层:    pet_fsm（FSM 情绪引擎）  gfx_engine（图形原语）
驱动层:      ssd1306_i2c  w25q_spi  sensor  buzzer  flash_config
             hc05（蓝牙单字符命令）  esp8266（WiFi 中断接收 + MQTT 桥接）
HAL层:       STM32CubeF1（GPIO / I²C / SPI / ADC / TIM / USART1/2/3）
```

全部驱动基于 STM32 HAL 库手写实现。裸机前后台架构，无 RTOS。USART1/2 轮询接收，USART3（ESP-01S）中断接收 + 环形缓冲区。

---

## 开发环境与编译

- **IDE**: Keil MDK-ARM v5，启用 MicroLIB
- **代码生成**: STM32CubeMX（工程文件 `stm32dog.ioc`）
- **源码目录**: `Core/Src/`（源文件）、`Core/Inc/`（头文件）
- **核心代码量**: ~2,500 行（不含 HAL 库）
- **Flash 占用**: ~60 KB（含 5 套动画点阵数据）

---

## 文件结构

```
stm32dog/
├── Core/
│   ├── Inc/
│   │   ├── ssd1306_i2c.h  w25q_spi.h  sensor.h  buzzer.h
│   │   ├── gfx_engine.h   pet_fsm.h   flash_config.h
│   │   ├── hc05.h  esp8266.h      # 蓝牙/WiFi 驱动
│   │   ├── cat_fullscreen.h  cat_anim1.h  cat_anim2.h
│   │   ├── cat_sleep.h  cat_hot.h
│   │   └── adc.h  gpio.h  i2c.h  spi.h  tim.h  usart.h
│   └── Src/
│       ├── main.c                # 主循环调度
│       ├── ssd1306_i2c.c  w25q_spi.c  sensor.c  buzzer.c
│       ├── gfx_engine.c   pet_fsm.c   flash_config.c
│       ├── hc05.c  esp8266.c      # 蓝牙/WiFi 驱动
│       ├── cat_fullscreen.c  cat_anim1.c  cat_anim2.c
│       ├── cat_sleep.c  cat_hot.c
│       └── adc.c  gpio.c  i2c.c  spi.c  tim.c  usart.c  stm32f1xx_it.c
├── Tools/
│   ├── esp8266_mqtt/              # ESP-01S Arduino MQTT 固件
│   ├── convert_fullscreen.py      # 点阵数据格式转换
│   ├── reduce_frames.py           # 帧数裁剪
│   └── gen_cat.py  gen_faces.py  gen_sprites.py  img2c.py
├── Drivers/                       # STM32CubeF1 HAL + CMSIS
├── MDK-ARM/                       # Keil 工程文件
├── README.md
└── stm32dog.ioc                   # CubeMX 工程配置
```

---

## 进度

| 模块 | 状态 |
|------|------|
| OLED 显示 + 5 状态动画 | ✅ |
| 传感器（LDR + NTC）+ ADC 滤波 | ✅ |
| 蜂鸣器 + LED 情绪联动 | ✅ |
| W25Q Flash 配置持久化 | ✅ |
| CH340 串口命令 | ✅ |
| HC-05 蓝牙遥控 + 传感器查询 + 阈值调节 | ✅ |
| ESP-01S WiFi + MQTT 远程控制 | ✅ 联调完成，USART3 中断接收 + 非阻塞重连 |

## 待做

- [ ] OLED 对比度自动调节
- [ ] MCU 低功耗 STOP 模式
