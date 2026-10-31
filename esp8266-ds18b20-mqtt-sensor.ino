/*
 * ESP8266 DS18B20 MQTT Temperature Node
 * -------------------------------------
 * Reads one or more DS18B20 waterproof probes on a single 1-Wire pin
 * every 30 seconds and publishes the temperatures as JSON to an MQTT
 * broker. Handy for aquaculture ponds, tanks and incubator prototypes.
 *
 * Required libraries (install via Library Manager):
 *   - "OneWire" by Paul Stoffregen
 *   - "DallasTemperature" by Miles Burton
 *   - "PubSubClient" by Nick O'Leary
 *
 * Board: any ESP8266 board (NodeMCU 1.0 / Wemos D1 mini)
 *
 * Wiring (1-Wire):
 *   DS18B20 red    (VDD)  -> ESP8266 3V3
 *   DS18B20 black  (GND)  -> ESP8266 GND
 *   DS18B20 yellow (DATA) -> GPIO4 / D2, with a 4.7k resistor between DATA and 3V3
 *   (Extra probes share the same three wires.)
 *
 * Setup:
 *   1. Copy config.h.example to config.h
 *   2. Edit config.h with your WiFi creds + MQTT broker details
 *   3. Flash. Open Serial Monitor at 115200.
 *
 * Sample MQTT message:
 *   {"count":2,"temps_c":[27.4,27.9],"uptime_s":120}
 *
 * (c) 2026 Fluidwire. Released under the MIT License.
 * https://github.com/fluidwire
 */

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include "config.h"

#define ONE_WIRE_PIN 4   // D2
#define MAX_PROBES   8

const uint32_t PUBLISH_INTERVAL_MS = 30000;

WiFiClient        wifiClient;
PubSubClient      mqtt(wifiClient);
OneWire           oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

uint32_t lastPublish = 0;

void connectWiFi() {
  Serial.printf("WiFi: connecting to %s ", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf(" OK, ip=%s\n", WiFi.localIP().toString().c_str());
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  while (!mqtt.connected()) {
    Serial.printf("MQTT: connecting to %s:%u as %s ... ",
                  MQTT_HOST, MQTT_PORT, MQTT_CLIENT_ID);
    bool ok = strlen(MQTT_USER) > 0
      ? mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)
      : mqtt.connect(MQTT_CLIENT_ID);
    if (ok) {
      Serial.println("OK");
    } else {
      Serial.printf("FAIL rc=%d, retry in 5s\n", mqtt.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  sensors.begin();
  int found = sensors.getDeviceCount();
  Serial.printf("1-Wire: found %d DS18B20 probe(s)\n", found);
  if (found == 0) {
    Serial.println("No probes found - check wiring and the 4.7k pull-up");
    while (true) delay(1000);
  }
  connectWiFi();
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  uint32_t now = millis();
  if (now - lastPublish < PUBLISH_INTERVAL_MS) return;
  lastPublish = now;

  sensors.requestTemperatures();  // blocks ~750 ms at 12-bit resolution
  int count = sensors.getDeviceCount();
  if (count > MAX_PROBES) count = MAX_PROBES;

  char payload[160];
  int n = snprintf(payload, sizeof(payload), "{\"count\":%d,\"temps_c\":[", count);
  int valid = 0;
  for (int i = 0; i < count; i++) {
    float t = sensors.getTempCByIndex(i);
    if (t == DEVICE_DISCONNECTED_C) {
      Serial.printf("Probe %d disconnected, skipping\n", i);
      continue;
    }
    n += snprintf(payload + n, sizeof(payload) - n, "%s%.1f", valid ? "," : "", t);
    valid++;
  }
  if (valid == 0) {
    Serial.println("No valid readings, skipping publish");
    return;
  }
  snprintf(payload + n, sizeof(payload) - n, "],\"uptime_s\":%lu}",
           (unsigned long)(now / 1000));

  if (mqtt.publish(MQTT_TOPIC, payload)) {
    Serial.printf("MQTT pub %s -> %s\n", MQTT_TOPIC, payload);
  } else {
    Serial.println("MQTT publish failed");
  }
}
