#include "receiver_MQTT_manager.h"
#include "WiFi.h"
#include <WiFiUdp.h>

namespace MQTT_manager {

int attemptTimes = 5;
bool mqttConnected = false;
WiFiClient netClient;
WiFiClientSecure tlsClient;
MQTTClient client;
int QoS_Val = 1;  // 0, 1, 2
const char* clientIdPrefix = "Hapbeat_esp32_client-";
void (*mqttCallback)(char*, byte*, unsigned int);
void (*statusCallback)(const char*);

void messageReceived(String& topic, String& payload) {
  if (mqttCallback) {
    mqttCallback((char*)topic.c_str(), (byte*)payload.c_str(),
                 payload.length());
  }
}

const char* ca_cert = MQTT_CERTIFICATE;
// ローカルブローカー解決（サブネット + 固定ホスト部）
static void mqtt_resolve_local_broker(char* outHost, size_t hostLen, int& outPort) {
  outPort = MQTT_LOCAL_PORT;
  IPAddress gw = WiFi.gatewayIP();
  IPAddress mask = WiFi.subnetMask();
  uint8_t net3 = gw[3] & mask[3];
  uint8_t hostMax = (~mask[3]) & 0xFF; // 下位オクテットの最大ホスト値
  uint8_t hostId = (uint8_t)(BROKER_STATIC_HOST_OCTET & 0xFF);
  if (hostMax <= 2 || hostId <= 1 || hostId >= hostMax) {
    hostId = (hostMax > 11) ? 10 : (hostMax > 2 ? (hostMax - 1) : 2);
  }
  uint8_t finalOctet = net3 | hostId;
  snprintf(outHost, hostLen, "%u.%u.%u.%u", gw[0], gw[1], gw[2], finalOctet);
}

String getUniqueClientId() {
  String clientId = clientIdPrefix;
  clientId += String(WiFi.macAddress());
  return clientId;
}

void reconnect() {
  int attemptCount = 0;  // 試行回数をカウントする変数
  while (!client.connected()) {
    String clientId = getUniqueClientId();
    String connectionAttemptMsg =
        "Connecting to \nMQTT trial: " + String(attemptCount);
    if (statusCallback) {
      statusCallback(connectionAttemptMsg.c_str());
    }

    if (client.connect(clientId.c_str(), MQTT_RECEIVER_USERNAME, MQTT_RECEIVER_PASSWORD)) {
      mqttConnected = true;
      if (statusCallback) {
        statusCallback("connected \nsuccess");
      }
      client.subscribe(MQTT_RECEIVER_TOPIC, QoS_Val);
      String subscribedMsg = "Subscribed topic: " + String(MQTT_RECEIVER_TOPIC);
      if (statusCallback) {
        statusCallback(subscribedMsg.c_str());
      }
    } else {
      String failMsg = "failed, rc=" + String(client.lastError());
      String retryMsg = "try again in 1 seconds";
    }
    delay(1000);
    if (attemptCount >= attemptTimes) {  // 10回試行したが接続できなかった場合
      statusCallback("connection failed");
      delay(3000);
      return;  // 接続失敗を報告して関数から抜ける
    }
    attemptCount++;  // 試行回数をインクリメント
  }
}

void initMQTTclient(void (*callback)(char*, byte*, unsigned int),
                    void (*statusCb)(const char*)) {
  mqttCallback = callback;
  statusCallback = statusCb;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attemptCount = 0;  // 試行回数をカウントする変数

  while (WiFi.status() != WL_CONNECTED) {
    String message = "Connecting to \nWiFi trial: " + String(attemptCount);
    statusCallback(message.c_str());  //
    // 現在の試行回数を含めてステータスを報告
    // statusCallback("Connecting to WiFi");  //
    // 現在の試行回数を含めてステータスを報告
    delay(1000);
    if (attemptCount >= 10) {  // 10回試行したが接続できなかった場合
      statusCallback("connection failed");
      break;  // 接続失敗を報告して関数から抜ける
    }
    attemptCount++;  // 試行回数をインクリメント
  }
  statusCallback("WiFi connected!");

  // mDNS の安定化のためスリープ無効
  WiFi.setSleep(false);
  // ソケットタイムアウト調整
  netClient.setTimeout(20000);
  tlsClient.setTimeout(20000);

  // ローカル/クラウド切替（ローカルは平文、クラウドはTLS）
  #if MQTT_USE_LOCAL_BROKER
    char host[32] = {0};
    int port = MQTT_LOCAL_PORT;
    mqtt_resolve_local_broker(host, sizeof(host), port);
    client.begin(host, port, netClient);
  #else
    tlsClient.setCACert(ca_cert);
    client.begin(MQTT_CLOUD_SERVER, MQTT_CLOUD_PORT, tlsClient);
  #endif
  // sender と同様のセッション運用（永続セッション）
  client.setCleanSession(false);
  // KeepAlive は既定値を使用
  client.onMessage(messageReceived);
  reconnect();
}

void loopMQTTclient() {
  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
  }
}

// wifiの情報を返す
bool getIsWiFiConnected() { return WiFi.status() == WL_CONNECTED; }

}  // namespace MQTT_manager
