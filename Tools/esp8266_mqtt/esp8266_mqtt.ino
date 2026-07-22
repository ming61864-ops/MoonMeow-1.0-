/**
 * ESP-01S 公共 MQTT Broker 测试固件
 * ================================
 *
 * 作用: WiFi联网 + MQTT连接公共broker + STM32串口桥接
 * 默认broker: broker.emqx.io (测试用, 无需注册)
 *
 * STM32 ←→ ESP-01S 协议 (USART3, 115200):
 *   STM32→ESP: STATUS,<情绪>,<LDR>,<NTC>\n   → publish到云端
 *   ESP→STM32: CMD,<TOUCH|FEED|SLEEP>\n     ← 云端下发
 *
 * 刷写方法:
 *   1. CH340 接 ESP-01S (TX→RX, RX→TX, VCC→3.3V, GND→GND)
 *   2. GPIO0 接 GND (进入下载模式)
 *   3. RST 拉低再拉高 (复位进入下载模式)
 *   4. Arduino IDE: 选 Generic ESP8266 Module, 115200, 1M(64K SPIFFS)
 *   5. 上传完成 → GPIO0 恢复接 3.3V → 复位
 *
 * 依赖库 (Arduino Library Manager):
 *   - PubSubClient (by Nick O'Leary)
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/* ============ 用户配置 (修改这里!) ============ */
const char* WIFI_SSID     = "yqc";
const char* WIFI_PASS     = "12345678";

/* MQTT 公共 broker (测试用) */
const char* MQTT_HOST = "broker.emqx.io";
const int   MQTT_PORT = 1883;

/* Topic 前缀, 建议改成你自己的唯一标识, 避免和别人冲突 */
const char* TOPIC_PREFIX  = "winner_mooncat";
/* ============================================= */

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

char topicCmd[64];
char topicStatus[64];

/* ---- MQTT 连接 (非阻塞) ---- */

static void connectMQTT()
{
    if (mqttClient.connected()) return;

    String clientId = "mooncat-" + String(ESP.getChipId(), HEX);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setKeepAlive(60);

    Serial.print("[MQTT] Connecting... RSSI=");
    Serial.print(WiFi.RSSI());
    Serial.print(" ");
    if (mqttClient.connect(clientId.c_str())) {
        Serial.println("OK");
        mqttClient.subscribe(topicCmd);
        Serial.println("[MQTT] Subscribed");
    } else {
        Serial.print("FAILED, rc=");
        Serial.println(mqttClient.state());
    }
}

/* ---- MQTT 消息回调 ---- */

static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    String cmd;
    for (unsigned int i = 0; i < length; i++) {
        cmd += (char)payload[i];
    }
    cmd.trim();
    cmd.toUpperCase();

    if (cmd == "TOUCH" || cmd == "T") {
        Serial.println("CMD,TOUCH");
    } else if (cmd == "FEED" || cmd == "F") {
        Serial.println("CMD,FEED");
    } else if (cmd == "SLEEP" || cmd == "Z") {
        Serial.println("CMD,SLEEP");
    }
}

/* ---- 上行: 发布状态到云端 ---- */

static void publishStatus(const char* mood, int ldr, int ntc)
{
    String payload = "{\"mood\":\"" + String(mood) + "\",";
    payload += "\"light\":" + String(ldr) + ",";
    payload += "\"temp\":" + String(ntc) + "}";

    /* 诊断: 收到STATUS行的时刻 + MQTT连接状态 + publish结果 */
    Serial.print("[ESP-RECV] tick=");
    Serial.print(millis());
    Serial.print(" mqttConnected=");
    Serial.print(mqttClient.connected());

    if (mqttClient.connected()) {
        bool ok = mqttClient.publish(topicStatus, payload.c_str());
        Serial.print(" publishOk=");
        Serial.println(ok);
    } else {
        Serial.println(" publishOk=SKIP(not connected)");
    }
}

/* ---- 串口数据解析 (来自 STM32) ---- */

static void processSerial()
{
    static String line = "";

    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            line.trim();
            if (line.length() > 0) {
                /* 解析: STATUS,<mood>,<ldr>,<ntc> */
                if (line.startsWith("STATUS,")) {
                    int comma1 = line.indexOf(',', 7);
                    int comma2 = line.indexOf(',', comma1 + 1);
                    if (comma1 > 0 && comma2 > 0) {
                        String mood = line.substring(7, comma1);
                        int ldr = line.substring(comma1 + 1, comma2).toInt();
                        int ntc = line.substring(comma2 + 1).toInt();
                        publishStatus(mood.c_str(), ldr, ntc);
                    }
                } else {
                    /* 非STATUS行也转发到MQTT, 方便调试 */
                    mqttClient.publish(topicStatus, line.c_str());
                }
            }
            line = "";
        } else if (c != '\r') {
            line += c;
        }
    }
}

/* ==================== 主程序 ==================== */

void setup()
{
    Serial.begin(115200);
    Serial.println("\n[ESP] Booting MoonCat WiFi...");

    /* 组装 topic */
    snprintf(topicCmd, sizeof(topicCmd), "%s/cmd", TOPIC_PREFIX);
    snprintf(topicStatus, sizeof(topicStatus), "%s/status", TOPIC_PREFIX);

    /* WiFi 连接 */
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("[WiFi] Connecting");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 60) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n[WiFi] FAILED");
    }

    /* MQTT */
    mqttClient.setCallback(mqttCallback);
    connectMQTT();
}

void loop()
{
    /* MQTT 保活 (非阻塞: 断线后每2秒重试一次, 不阻塞 processSerial) */
    static uint32_t lastReconnect = 0;
    if (!mqttClient.connected()) {
        uint32_t nowMs = millis();
        if (nowMs - lastReconnect >= 2000) {
            lastReconnect = nowMs;
            connectMQTT();
        }
    }
    mqttClient.loop();

    /* 处理 STM32 发来的数据 */
    processSerial();

    delay(10);
}
