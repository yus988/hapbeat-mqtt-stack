#include "sender_MQTT_manager.h"
#include <WiFiUdp.h>

namespace MQTT_manager {

bool mqttConnected = false;
WiFiClient netClient;
WiFiClientSecure tlsClient;
MQTTClient client(256);
const char* clientIdPrefix = "Sensor_esp32_client-";
const char* ca_cert = MQTT_CERTIFICATE;
void (*statusCallback)(const char*);

// トピック名の定義は config.h から取得
const char* topicHapbeat = MQTT_TOPIC_HAPBEAT;
const char* topicWebApp = MQTT_TOPIC_WEBAPP;
const char* topicColor = MQTT_TOPIC_COLOR;
const int QoS_Val = 1;  // 0=once, 1=least once, 2=exact once

// ローカルブローカー解決（サブネット + 固定ホスト部）
static void mqtt_resolve_local_broker(char* outHost, size_t hostLen, int& outPort) {
  outPort = MQTT_LOCAL_PORT;
  IPAddress gw = WiFi.gatewayIP();
  IPAddress mask = WiFi.subnetMask();
  uint8_t net3 = gw[3] & mask[3];
  uint8_t hostMax = (~mask[3]) & 0xFF; // 利用可能ホスト最大値（下位オクテット）
  uint8_t hostId = (uint8_t)(BROKER_STATIC_HOST_OCTET & 0xFF);
  if (hostMax <= 2 || hostId <= 1 || hostId >= hostMax) {
    hostId = (hostMax > 11) ? 10 : (hostMax > 2 ? (hostMax - 1) : 2);
  }
  uint8_t finalOctet = net3 | hostId;
  snprintf(outHost, hostLen, "%u.%u.%u.%u", gw[0], gw[1], gw[2], finalOctet);
}

// ユニークなクライアントIDを生成する関数
String getUniqueClientId() {
  String clientId = clientIdPrefix;
  clientId += String(WiFi.macAddress());
  return clientId;
}

// MQTTメッセージ到着時のコールバック関数
void messageReceived(String& topic, String& payload) {
  Serial.println("Message arrived: ");
  Serial.println("  topic: " + topic);
  Serial.println("  payload: " + payload);
}

// MQTTブローカーへの接続関数
void connect() {
  while (!client.connected()) {
    String clientId = getUniqueClientId();
    Serial.print("Attempting MQTT connection with client ID: ");
    Serial.println(clientId);

    if (statusCallback) {
      statusCallback("Attempting MQTT connection...");
    }

    if (client.connect(clientId.c_str(), MQTT_SENDER_USERNAME, MQTT_SENDER_PASSWORD)) {
      Serial.println("connected");
      mqttConnected = true;
      if (statusCallback) {
        statusCallback("connected success");
      }
      // サブスクリプションを再設定（必要なら追加）
      if (client.subscribe(topicHapbeat, 1)) {
        Serial.print("Subscribed to topic: ");
        Serial.println(topicHapbeat);
        statusCallback("Successfully connected to Hapbeat");
      } else {
        mqttConnected = false;
        Serial.println("Subscription to topicHapbeat failed");
      }
      #if !MQTT_USE_LOCAL_BROKER
      if (client.subscribe(topicWebApp, 1)) {
        Serial.print("Subscribed to topic: ");
        Serial.println(topicWebApp);
      } else {
        mqttConnected = false;
        Serial.println("Subscription to topicWebApp failed");
      }
      #endif
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.lastError());
      Serial.println(" try again in 5 seconds");

      mqttConnected = false;
      if (statusCallback) {
        statusCallback("connect failed");
      }
      delay(5000);
    }
  }
}

void initMQTTclient(void (*statusCb)(const char*)) {
  statusCallback = statusCb;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // ローカルブローカーは平文のため TLS は無効、クラウド時のみ証明書設定
  netClient.setTimeout(20000);
  tlsClient.setTimeout(20000);

  // ローカル/クラウド自動切替（ローカル優先）
  client.setCleanSession(false);
  #if MQTT_USE_LOCAL_BROKER
    char host[32] = {0};
    int port = MQTT_LOCAL_PORT;
    mqtt_resolve_local_broker(host, sizeof(host), port);
    Serial.printf("WiFi IP=%s GW=%s MASK=%s\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.subnetMask().toString().c_str());
    Serial.printf("MQTT host=%s port=%d\n", host, port);
    client.begin(host, port, netClient);
  #else
    tlsClient.setCACert(ca_cert);
    Serial.printf("WiFi IP=%s GW=%s MASK=%s\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.subnetMask().toString().c_str());
    Serial.printf("MQTT host=%s port=%d (TLS)\n", MQTT_CLOUD_SERVER, MQTT_CLOUD_PORT);
    client.begin(MQTT_CLOUD_SERVER, MQTT_CLOUD_PORT, tlsClient);
  #endif
  // client.onMessage(messageReceived);
  connect();

  // 誤って retain 送ってしまった場合にリセット
    // client.publish(topicHapbeat, "", true,
    //               QoS_Val);  // 空のペイロードとRetainedフラグをtrueに設定
    // client.publish(topicWebApp, "", true,
    //               QoS_Val);  // 空のペイロードとRetainedフラグをtrueに設定
}

void loopMQTTclient() {
  if (!client.connected()) {
    connect();
  }
  client.loop();
}

// トピックごとのメッセージ送信関数
void sendMessageToHapbeat(const char* message) {
  if (client.publish(topicHapbeat, message, true, QoS_Val)) {
    Serial.print("Message sent to Hapbeat: ");
    Serial.println(message);
  } else {
    Serial.println("Failed to send message to Hapbeat");
  }
}

void sendMessageToWebApp(const char* message) {
  #if MQTT_USE_LOCAL_BROKER
  (void)message; // local brokerではWebApp不要
  #else
  if (client.publish(topicWebApp, message, false, QoS_Val)) {
    Serial.print("Message sent to WebApp: ");
    Serial.println(message);
  } else {
    Serial.println("Failed to send message to WebApp");
  }
  #endif
}

// color 名のみ送信（brokerが色を直接表示できるように）
void sendColorName(const char* colorName) {
  client.publish(topicColor, colorName, false, QoS_Val);
}

}  // namespace MQTT_manager
