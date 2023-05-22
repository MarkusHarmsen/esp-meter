#include "main.h"

/*
 * MQTT Section
 */
bool mqttConnect() {
  if (client.connect(MQTT_CLIENT_IDENTIFIER)) {
    #if METER_WATER
      waterMeter.publishHomeAssistantAutoDiscovery();
    #endif
    #if METER_GAS
      gasMeter.publishHomeAssistantAutoDiscovery();
    #endif
    #if METER_POWER
      powerMeter.publishHomeAssistantAutoDiscovery();
    #endif
  }

  return client.connected();
}

long mqttLastConnectionAttempt = 0;
void mqttLoop() {
  if (!client.connected()) {
    if (millis() - mqttLastConnectionAttempt > 5000) {
      mqttLastConnectionAttempt = millis();
      if (mqttConnect()) {
        mqttLastConnectionAttempt = 0;
      }
    }
  } else {
    client.loop();
  }
}

/*
 * Main Stuff
 */
void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(SSID, PSK);

  client.setKeepAlive(60);
  client.setBufferSize(512);
  client.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
  client.connect(MQTT_CLIENT_IDENTIFIER);

#if METER_WATER
  waterMeter.setup();
#endif
#if METER_GAS
  gasMeter.setup();
#endif
#if METER_POWER
  powerMeter.setup();
#endif
#if EASTRON_SIMULATOR
  eastronSimulator.setup();
#endif

  /*
   * Timers - added a few number offsets to not have them run in the same loop often.
   */
  timer.every(50 + 1, [](void*) -> bool {
    mqttLoop();
    return true;
  });

#if METER_WATER || METER_GAS
  timer.every(500 + 17, [](void*) -> bool {
  #if METER_WATER
    if (waterMeter.hasChangesToBePublished()) { waterMeter.publishNow(); }
  #endif
  #if METER_GAS
    if (gasMeter.hasChangesToBePublished()) { gasMeter.publishNow(); }
  #endif
    return true;
  });
#endif

#if METER_POWER
  timer.every(1000 + 9, [](void*) -> bool {
    powerMeter.publishNow();
    return true;
  });
#endif

#if METER_WATER || METER_GAS
  timer.every(10000 + 11, [](void*) -> bool {
  #if METER_WATER
    waterMeter.publishNow();
  #endif
  #if METER_GAS
    gasMeter.publishNow();
  #endif
    return true;
  });
#endif
}

void loop() {
#if METER_POWER
  powerMeter.loop();

  // Fast update
  if (powerMeter.currentPowerUpdated) {
    powerMeter.currentPowerUpdated = false;
  #if EASTRON_SIMULATOR
    auto power = powerMeter.getCurrentPower();
  #if defined(EASTRON_SIMULATOR_CAP_MIN)
    if (power < EASTRON_SIMULATOR_CAP_MIN) { power = EASTRON_SIMULATOR_CAP_MIN; }
  #endif
  #if defined(EASTRON_SIMULATOR_CAP_MAX)
    if (power > EASTRON_SIMULATOR_CAP_MAX) { power = EASTRON_SIMULATOR_CAP_MAX; }
  #endif
    eastronSimulator.setCurrentPower(power, powerMeter.getReadingsInStable(), powerMeter.getReadingsOutStable(), powerMeter.getCurrentVoltage());
  #endif
  }
#endif
#if EASTRON_SIMULATOR
  eastronSimulator.loop();
#endif

#if METER_WATER
  waterMeter.loop();
#endif

#if METER_GAS
  gasMeter.loop();
#endif

  // Go for the timers
  timer.tick();

  delay(1);
}
