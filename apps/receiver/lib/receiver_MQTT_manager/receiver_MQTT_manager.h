#ifndef RECEIVER_MQTT_MANAGER_H
#define RECEIVER_MQTT_MANAGER_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include "config.h"

namespace MQTT_manager {
extern bool mqttConnected;
extern WiFiClient netClient;       // 平文 (ローカル)
extern WiFiClientSecure tlsClient; // TLS (クラウド)
bool getIsWiFiConnected();
void initMQTTclient(void (*callback)(char*, byte*, unsigned int),
                    void (*statusCb)(const char*)) ;
void loopMQTTclient();
}  // namespace MQTT_manager

#endif  // RECEIVER_MQTT_MANAGER_H
