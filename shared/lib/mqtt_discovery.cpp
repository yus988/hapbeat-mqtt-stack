#include "mqtt_discovery.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include "../../config.h"

bool mqtt_resolve_local_broker(char* outHost, size_t hostLen, int& outPort) {
  outHost[0] = 0;
  outPort = MQTT_LOCAL_PORT;
  // 1) Fixed IP
  if (String(MQTT_LOCAL_IP).length() > 0) {
    strlcpy(outHost, MQTT_LOCAL_IP, hostLen);
    return true;
  }
  // 2) UDP discovery
  WiFiUDP u;
  u.begin(0);
  IPAddress bcast = IPAddress(~WiFi.subnetMask() | WiFi.gatewayIP());
  u.beginPacket(bcast, 53531);
  u.write((const uint8_t*)"HB_DISCOVER", 11);
  u.endPacket();
  unsigned long t0 = millis();
  char rbuf[64] = {0};
  while (millis() - t0 < 500) {
    int p = u.parsePacket();
    if (p) {
      int n = u.read(rbuf, sizeof(rbuf) - 1);
      if (n > 0) rbuf[n] = 0;
      if (strncmp(rbuf, "MQTT:", 5) == 0) {
        char* ipstr = rbuf + 5;
        char* colon = strchr(ipstr, ':');
        if (colon) { *colon = 0; outPort = atoi(colon + 1); }
        strlcpy(outHost, ipstr, hostLen);
        return true;
      }
    }
    delay(10);
  }
  // 3) mDNS fallback
  strlcpy(outHost, String(MQTT_LOCAL_HOSTNAME ".local").c_str(), hostLen);
  return true;
}


