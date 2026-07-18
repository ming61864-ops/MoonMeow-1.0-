/**
 * ESP-01S 阿里云 IoT MQTT 固件
 * ================================
 *
 * 作用: WiFi联网 + MQTT连接阿里云IoT + STM32串口桥接
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
const char* WIFI_SSID     = "你的WiFi名";
const char* WIFI_PASS     = "你的WiFi密码";

/* 阿里云 IoT 三元组 (控制台 → 设备详情) */
const char* PRODUCT_KEY   = "你的ProductKey";
const char* DEVICE_NAME   = "你的DeviceName";
const char* DEVICE_SECRET = "你的DeviceSecret";

/* MQTT 服务器地址 (根据你的实例区域调整) */
const char* MQTT_HOST = "你的ProductKey.iot-as-mqtt.cn-shanghai.aliyuncs.com";
const int   MQTT_PORT = 1883;
/* ============================================= */

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

/* ---- HMAC-SHA1 实现 ---- */

#include <Hash.h>

static String hmacSha1(const String &key, const String &data)
{
    uint8_t hmac_key[128];
    const int BLOCK_SIZE = 64;
    int key_len = key.length();

    memset(hmac_key, 0, BLOCK_SIZE);
    if (key_len > BLOCK_SIZE) {
        sha1(key.c_str(), key_len, hmac_key);
        key_len = 20;
    } else {
        memcpy(hmac_key, key.c_str(), key_len);
    }

    /* ipad / opad */
    uint8_t ipad[BLOCK_SIZE + 64], opad[BLOCK_SIZE + 64];
    for (int i = 0; i < BLOCK_SIZE; i++) {
        ipad[i] = hmac_key[i] ^ 0x36;
        opad[i] = hmac_key[i] ^ 0x5C;
    }
    memcpy(ipad + BLOCK_SIZE, data.c_str(), data.length());
    memcpy(opad + BLOCK_SIZE, data.c_str(), data.length());  /* placeholder */

    uint8_t inner_hash[20];
    sha1(ipad, BLOCK_SIZE + data.length(), inner_hash);

    memcpy(opad + BLOCK_SIZE, inner_hash, 20);
    uint8_t result[20];
    sha1(opad, BLOCK_SIZE + 20, result);

    String hex;
    for (int i = 0; i < 20; i++) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02X", result[i]);
        hex += buf;
    }
    return hex;
}

/* ---- MQTT 连接 ---- */

static void connectMQTT()
{
    if (mqttClient.connected()) return;

    String clientId = String(DEVICE_NAME) + "|securemode=3,signmethod=hmacsha1|";
    String username  = String(DEVICE_NAME) + "&" + PRODUCT_KEY;
    String password  = hmacSha1(DEVICE_SECRET, clientId);

    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setKeepAlive(60);

    Serial.print("[MQTT] Connecting...");
    if (mqttClient.connect(clientId.c_str(), username.c_str(), password.c_str())) {
        Serial.println("OK");

        /* 订阅云端下发的属性设置和命令 */
        String topic1 = "/sys/" + String(PRODUCT_KEY) + "/" + DEVICE_NAME + "/thing/service/property/set";
        String topic2 = "/sys/" + String(PRODUCT_KEY) + "/" + DEVICE_NAME + "/thing/service/touch";
        String topic3 = "/sys/" + String(PRODUCT_KEY) + "/" + DEVICE_NAME + "/thing/service/feed";
        String topic4 = "/sys/" + String(PRODUCT_KEY) + "/" + DEVICE_NAME + "/thing/service/sleep";

        mqttClient.subscribe(topic1.c_str());
        mqttClient.subscribe(topic2.c_str());
        mqttClient.subscribe(topic3.c_str());
        mqttClient.subscribe(topic4.c_str());
        Serial.println("[MQTT] Subscribed");
    } else {
        Serial.print("FAILED, rc=");
        Serial.println(mqttClient.state());
    }
}

/* ---- MQTT 消息回调 ---- */

static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    /* 提取 topic 尾部 (最后一个 / 之后) 识别服务名 */
    String t(topic);
    int lastSlash = t.lastIndexOf('/');
    String service = (lastSlash >= 0) ? t.substring(lastSlash + 1) : t;

    /* 属性设置 → 解析指令 */
    if (service == "set") {
        String json;
        json.concat((char*)payload, length);
        /* 简单子串匹配, 不引入 JSON 库 */
        if (json.indexOf("\"touch\"") > 0 || json.indexOf("TOUCH") > 0) {
            Serial.println("CMD,TOUCH");
        } else if (json.indexOf("\"feed\"") > 0 || json.indexOf("FEED") > 0) {
            Serial.println("CMD,FEED");
        } else if (json.indexOf("\"sleep\"") > 0 || json.indexOf("SLEEP") > 0) {
            Serial.println("CMD,SLEEP");
        }
    }
    /* 直接服务调用 */
    else if (service == "touch") {
        Serial.println("CMD,TOUCH");
    }
    else if (service == "feed") {
        Serial.println("CMD,FEED");
    }
    else if (service == "sleep") {
        Serial.println("CMD,SLEEP");
    }
}

/* ---- 上行: 发布状态到云端 ---- */

static void publishStatus(const char* mood, int ldr, int ntc)
{
    String topic = "/sys/" + String(PRODUCT_KEY) + "/" + DEVICE_NAME + "/thing/event/property/post";
    String payload = "{\"id\":\"1\",\"params\":{";
    payload += "\"mood\":{\"value\":\"" + String(mood) + "\"},";
    payload += "\"light\":{\"value\":" + String(ldr) + "},";
    payload += "\"temp\":{\"value\":" + String(ntc) + "}";
    payload += "},\"method\":\"thing.event.property.post\"}";

    if (mqttClient.connected()) {
        mqttClient.publish(topic.c_str(), payload.c_str());
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
    /* MQTT 保活 */
    if (!mqttClient.connected()) {
        delay(2000);
        connectMQTT();
    }
    mqttClient.loop();

    /* 处理 STM32 发来的数据 */
    processSerial();

    delay(10);
}
