#ifndef MQTT_DISCOVERY_H
#define MQTT_DISCOVERY_H

#include <Arduino.h>

// 解析結果を outHost/outPort に設定して true を返す。
// 優先順: MQTT_LOCAL_IP -> UDP DISCOVERY -> mDNS(host.local)
bool mqtt_resolve_local_broker(char* outHost, size_t hostLen, int& outPort);

#endif

