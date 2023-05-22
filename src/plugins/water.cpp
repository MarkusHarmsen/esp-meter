#include "water.h"

void WaterMeter::publishNow() {
  publishCounter(HA_TOPIC_WATER_COUNTER, pulseCounter);
  publishFloat(HA_TOPIC_WATER_CONSUMPTION_M3, pulseCounter * 0.001);

#if DEBUGGING
  publishFloat("meter_esp/water/debug/low", debugLowestValue);
  publishFloat("meter_esp/water/debug/high", debugHighestValue);
  publishFloat("meter_esp/water/debug/current", samples.getAverage());
#endif

  didPublish();
}

void WaterMeter::onValue(uint16_t value) {
  samples.add(value);
  if (!samples.isFull()) { return; } // Wait for more data

  auto avg = samples.getAverage();

#if DEBUGGING
  if (avg < debugLowestValue) { debugLowestValue = avg; }
  if (avg > debugHighestValue) { debugHighestValue = avg; }
#endif

  // Check if we a distinct high or low band
  bool currentlyHigh = avg > thresholdHigh;
  if (avg > thresholdHigh) { wasHigh = true; }
  if (avg < thresholdLow) { wasLow = true; }

  // Trigger Pulse if we have been high, have been low and are currently high
  if (wasHigh && wasLow && currentlyHigh) {
    pulseCounter++;
    wasHigh = wasLow = false;
    setDirty();

  #if DEBUGGING
    // Reset Debugging each 10 pulses
    if (pulseCounter % 10 == 0) { resetDebugging(); }
  #endif
  }
}

void WaterMeter::setup() {
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, LOW);
#if defined(ESP32)
  analogReadResolution(8);
#endif

#if DEBUGGING
  resetDebugging();
#endif
}

#if DEBUGGING
void WaterMeter::resetDebugging() {
  debugHighestValue = 0;
  debugLowestValue = 4096; // Highest possible adc read
}
#endif

void WaterMeter::loop() {
  auto now = millis();
  if (now < nextActionAt) { return; }

  if (nextAction == MEASURE_OFF) {
    lastMeasureOffValue = analogRead(pinAdc);

    // Turn LED on
    digitalWrite(pinLed, HIGH);
    nextAction = MEASURE_ON;
    nextActionAt = now + 10;
  } else if (nextAction == MEASURE_ON) {
    uint16_t currentValue = analogRead(pinAdc);

    // Turn LED off
    digitalWrite(pinLed, LOW);
    nextAction = MEASURE_OFF;
    nextActionAt = now + 100;

    if (currentValue >= lastMeasureOffValue) {
      onValue(currentValue - lastMeasureOffValue);
    }
  }
}
