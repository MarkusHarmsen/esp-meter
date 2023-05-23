#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
  #include <SoftwareSerial.h>
#endif
#include <PubSubClient.h>
#include <arduino-timer.h>

// Enable plugins
#define METER_WATER true
#define METER_GAS true
#define METER_POWER true
#define EASTRON_SIMULATOR defined(ESP32) && true
#define EASTRON_SIMULATOR_CAP_MIN -320

#include "secrets.h"
#ifndef SECRETS
#define SECRETS
// You can define these constants in "secrets.h"
const char* SSID = "YOUR_WIFI_SSID";
const char* PSK = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER = "192.168.1.1";
const int   MQTT_BROKER_PORT = 1883;
const char* MQTT_CLIENT_IDENTIFIER = "ESPMETER";
#endif

WiFiClient espClient;
PubSubClient client(espClient);

#if METER_WATER
  #include "plugins/water.h"

  /*
    Config (in order)
      - MQTT Client
      - GPIO: Emitter LED
      - GPIO: Photodiode ADC
      - Lower threshold
      - Higher threshold
  */
  WaterMeter waterMeter(&client, 32, 33, 80, 120);
#endif
#if METER_GAS
  #include "plugins/gas.h"

  /*
    Config (in order)
      - MQTT Client
      - GPIO: Contact-Switch
      - Factor to calculate counter into kWH, e.g. "Abrechnungsbrennwert 11,500 kWh/m3 * Zustandszahl 0,9529"
  */
  GasMeter gasMeter(&client, 21, 11.500 * 0.9529);
#endif
#if METER_POWER
  #include "plugins/power.h"

  /*
    Config (in order)
      - MQTT Client
      - Serial Communication (only RX is used)
  */
  PowerMeter powerMeter(&client, &Serial);
#endif
#if EASTRON_SIMULATOR
  #include "plugins/eastron_simulator.h"

  /*
    Config (in order)
      - Serial Communication (RX/TX is used)
  */
  EastronSimulator eastronSimulator(&Serial2);
#endif

Timer<5> timer;
