#include "gas.h"

void GasMeter::publishNow() {
  publishCounter(HA_TOPIC_GAS_COUNTER, pulseCounter);

  float consumptionM3 = pulseCounter * 0.1;
  publishFloat(HA_TOPIC_GAS_CONSUMPTION_M3, consumptionM3);

  float consumptionKWh = consumptionM3 * m3ToKWhFactor;
  publishFloat(HA_TOPIC_GAS_CONSUMPTION_KWH, consumptionKWh);

  didPublish();
}

void GasMeter::setup() {
  pinMode(pin, INPUT_PULLUP);

  bounce.attach(pin);
  bounce.interval(50);
}

void GasMeter::loop() {
  auto now = millis();
  if (now < nextActionAt) { return; }

  nextActionAt = now + 20;
  bounce.update();
  
  // Trigger Pulse
  if (bounce.fell()) {
    pulseCounter++;
    setDirty();
  }
}