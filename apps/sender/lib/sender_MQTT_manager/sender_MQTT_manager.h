#ifndef SENDER_MQTT_MANAGER_H
#define SENDER_MQTT_MANAGER_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>  // 256dpi/MQTTライブラリをインクルード

#include "config.h"  // ルートの config を直接参照（-I . で解決）

namespace MQTT_manager {

extern bool mqttConnected;
extern WiFiClient netClient;       // 平文 (ローカル)
extern WiFiClientSecure tlsClient; // TLS (クラウド)
extern MQTTClient client;
extern void (*statusCallback)(const char*);

void initMQTTclient(void (*statusCb)(const char*));
void loopMQTTclient();

// メッセージ送信関数の宣言
void sendMessageToHapbeat(const char* message);
void sendMessageToWebApp(const char* message);

}  // namespace MQTT_manager

#endif  // SENDER_MQTT_MANAGER_H
