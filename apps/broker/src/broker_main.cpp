#include <WiFi.h>
#include <M5Unified.h>
#include "sMQTTBroker.h"
#include "config.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

// 文字を表示させる位置
int IP_posY = 0;
int port_posY = 12;
int device_posY = 24;
int info_posY = 52; // legacy (unused for bottom area now)
float text_size = 1.2f;


static int senderCount = 0;
static int receiverCount = 0;
static std::unordered_map<sMQTTClient*, int> clientRole; // 1: sender, 2: receiver
static std::vector<std::string> senderIds;
static std::vector<std::string> receiverIds;

static std::string shortId(const std::string& cid) {
    // コロンやハイフンなどの区切りを除去して末尾6文字を採用
    std::string s;
    s.reserve(cid.size());
    for (char c : cid) {
        if (c != ':' && c != '-' && c != ' ') s.push_back(c);
    }
    if (s.size() > 6) return s.substr(s.size() - 6);
    return s;
}

static int bottomReservePx() {
    return M5.Lcd.fontHeight() * 3; // reserve 3 lines at bottom
}

void drawIPandPort() {
    M5.Lcd.setTextColor(WHITE, BLACK);
    const int lh = M5.Lcd.fontHeight();
    M5.Lcd.fillRect(0, IP_posY, 320, lh, BLACK);
    M5.Lcd.fillRect(0, port_posY, 320, lh, BLACK);
    M5.Lcd.setCursor(0, IP_posY);
    M5.Lcd.printf("IP:%s\n", WiFi.localIP().toString().c_str());
    M5.Lcd.setCursor(0, port_posY);
    M5.Lcd.printf("Port:%d\n", MQTT_LOCAL_PORT);
}

void drawDeviceStatus() {
    M5.Lcd.setTextColor(WHITE, BLACK);
    const int lh = M5.Lcd.fontHeight();
    // 消去（下3行を残す）
    int h = M5.Lcd.height() - device_posY - bottomReservePx();
    if (h < 0) h = 0;
    M5.Lcd.fillRect(0, device_posY, 320, h, BLACK);
    int y = device_posY;
    M5.Lcd.setCursor(0, y);
    M5.Lcd.printf("S:%d R:%d\n", senderCount, receiverCount);
    y += lh;
    // sender list
    M5.Lcd.setCursor(0, y);
    M5.Lcd.print("sender\n");
    y += lh;
    for (auto &id : senderIds) {
        M5.Lcd.setCursor(0, y);
        M5.Lcd.printf("- %s\n", id.c_str());
        y += lh;
    }
    // receiver list
    M5.Lcd.setCursor(0, y);
    M5.Lcd.print("receiver\n");
    y += lh;
    for (auto &id : receiverIds) {
        M5.Lcd.setCursor(0, y);
        M5.Lcd.printf("- %s\n", id.c_str());
        y += lh;
    }
}

void redrawAll() {
    M5.Lcd.fillScreen(BLACK);
    drawIPandPort();
    drawDeviceStatus();
}

void drawCommunication(const char* sid, const char* colorName) {
    M5.Lcd.setTextColor(WHITE, BLACK);
    const int lh = M5.Lcd.fontHeight();
    int y = M5.Lcd.height() - bottomReservePx();
    // クリア下3行分
    M5.Lcd.fillRect(0, y, 320, bottomReservePx(), BLACK);
    M5.Lcd.setCursor(0, y);
    M5.Lcd.print("Communication\n");
    y += lh;
    M5.Lcd.setCursor(0, y);
    M5.Lcd.printf("- sensor ID: %s\n", sid);
    y += lh;
    M5.Lcd.setCursor(0, y);
    M5.Lcd.printf("- color: %s\n", colorName);
}

// UDP ディスカバリ応答
static WiFiUDP udp;
static const uint16_t DISCOVERY_PORT = 53531;
static const char* DISCOVERY_REQ = "HB_DISCOVER";
static const char* DISCOVERY_RES_PREFIX = "MQTT:"; // e.g. MQTT:192.168.0.2:1883

class MyBroker : public sMQTTBroker {
   public:
    bool onConnect(sMQTTClient *client, const std::string &username,
                   const std::string &password) {
        // 役割を記録
        std::string cid = client->getClientId();
        std::string sid = shortId(cid);
        if (username == std::string(MQTT_SENDER_USERNAME)) {
            clientRole[client] = 1; senderCount++;
            if (std::find(senderIds.begin(), senderIds.end(), sid) == senderIds.end()) {
                senderIds.push_back(sid);
            }
        } else if (username == std::string(MQTT_RECEIVER_USERNAME)) {
            clientRole[client] = 2; receiverCount++;
            if (std::find(receiverIds.begin(), receiverIds.end(), sid) == receiverIds.end()) {
                receiverIds.push_back(sid);
            }
        } else {
            clientRole[client] = 0;
        }
        // 画面全再描画（一覧やIP表示を更新）
        redrawAll();
        // 下部のCommunication領域は触らない
        return true;
    }

    void onRemove(sMQTTClient *client) {
        String message =
            "onRemove clientId=\n" + String(client->getClientId().c_str());
        // 役割を解放 & IDリストから削除
        auto it = clientRole.find(client);
        if (it != clientRole.end()) {
            if (it->second == 1 && senderCount > 0) senderCount--;
            if (it->second == 2 && receiverCount > 0) receiverCount--;
            std::string cid = client->getClientId();
            std::string sid = shortId(cid);
            if (it->second == 1) {
                auto sit = std::find(senderIds.begin(), senderIds.end(), sid);
                if (sit != senderIds.end()) senderIds.erase(sit);
            } else if (it->second == 2) {
                auto rit = std::find(receiverIds.begin(), receiverIds.end(), sid);
                if (rit != receiverIds.end()) receiverIds.erase(rit);
            }
            clientRole.erase(it);
        }
        // 画面全再描画（切り替え）
        redrawAll();
    }

    void onSubscribe(sMQTTClient *client, const std::string &topic, int qos) {
        String message = "Client subscribed to topic: " + String(topic.c_str());
        M5.Lcd.fillRect(0, info_posY, 320, 80, BLACK);  // 画面をクリア
        M5.Lcd.setCursor(10, info_posY);
        M5.Lcd.printf("%s\n", message.c_str());  // M5.Lcdに出力
    }

    void onPublish(sMQTTClient *client, const std::string &topic,
                   const std::string &payload) {
        // sender が MQTT_TOPIC_COLOR に color名のみを送信
        if (topic == std::string(MQTT_TOPIC_COLOR)) {
            std::string sid = shortId(client->getClientId());
            drawCommunication(sid.c_str(), payload.c_str());
        }
    }

    bool onEvent(sMQTTEvent *event) {
        switch (event->Type()) {
            case NewClient_sMQTTEventType: {
                sMQTTNewClientEvent *e = (sMQTTNewClientEvent *)event;
                e->Login();
                e->Password();
            } break;

            case LostConnect_sMQTTEventType:
                WiFi.reconnect();
                break;
        }
        return true;
    }
};

MyBroker broker;

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASSWORD;

void startWiFiClient() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, IP_posY);
    M5.Lcd.printf("Connecting...\n");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Lcd.print(".");  // 接続中のメッセージ
    }

    // mDNS を開始し、mqttサービスを登録
    if (!MDNS.begin(MQTT_LOCAL_HOSTNAME)) {
        // no-op if failed
    } else {
        MDNS.addService("mqtt", "tcp", MQTT_LOCAL_PORT);
    }
    // UDP 受信開始（ディスカバリ応答）
    udp.begin(DISCOVERY_PORT);
    redrawAll();
}

void setup() {
    M5.begin();                // M5Stackの初期化
    M5.Lcd.fillScreen(BLACK);  // 画面をクリア
    // 固定ピクセルフォント（元の小さい等幅に戻す）
    M5.Lcd.setFont(&fonts::Font0); // 6x8 等幅

    startWiFiClient();  // Wi-Fi接続
    const unsigned short mqttPort = 1883;
    broker.init(mqttPort);  // MQTTブローカーの初期化

    // ポート情報の表示
    M5.Lcd.fillRect(0, 80, 320, 80, BLACK);  // 画面をクリア
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.printf("MQTT broker started on \nport: %d\n", mqttPort);
}

void loop() {
    broker.update();  // ブローカーの更新
    M5.update();
    // UDP ディスカバリ: リクエスト受信でIP:PORTを返す
    int pkt = udp.parsePacket();
    if (pkt) {
        char buf[64] = {0};
        int len = udp.read(buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = 0;
            if (String(buf) == DISCOVERY_REQ) {
                IPAddress ip = WiFi.localIP();
                char resp[64];
                snprintf(resp, sizeof(resp), "%s%u.%u.%u.%u:%d", DISCOVERY_RES_PREFIX,
                         ip[0], ip[1], ip[2], ip[3], MQTT_LOCAL_PORT);
                udp.beginPacket(udp.remoteIP(), udp.remotePort());
                udp.write((const uint8_t*)resp, strlen(resp));
                udp.endPacket();
            }
        }
    }
}
