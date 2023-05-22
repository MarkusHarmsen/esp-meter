#include "meter.h"
#include <RunningMedian.h>

#define DEBUGGING true

#define HA_TOPIC_WATER_COUNTER "meter_esp/water/counter"
#define HA_TOPIC_WATER_CONSUMPTION_M3 "meter_esp/water/consumption/m3"

enum Action { MEASURE_OFF, MEASURE_ON };

class WaterMeter : public Meter {
  public:
    WaterMeter(PubSubClient *client, uint8_t pinLed, uint8_t pinAdc, uint16_t thresholdLow, uint16_t thresholdHigh) : Meter(client) {
      this->pinLed = pinLed;
      this->pinAdc = pinAdc;
      this->thresholdLow = thresholdLow;
      this->thresholdHigh = thresholdHigh;
    };

    void publishHomeAssistantAutoDiscovery() {
      publishAutoDiscovery(HA_SENSOR_PREFIX "water_counter/config", "{\"name\": \"Water Counter\", \"stat_t\": \"" HA_TOPIC_WATER_COUNTER "\", \"obj_id\": \"meter_water_counter\", \"stat_cla\": \"total_increasing\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "water_consumption/config", "{\"name\": \"Water Consumption\", \"stat_t\": \"" HA_TOPIC_WATER_CONSUMPTION_M3 "\", \"obj_id\": \"meter_water_consumption\", \"unit_of_meas\": \"m³\", \"stat_cla\": \"total_increasing\", \"dev_cla\": \"water\" }");
    };
    
    void publishNow();
    void setup();
    void loop();
    bool hasChangesToBePublished() { return dirty; }

  private:
    uint8_t pinLed, pinAdc;
    uint16_t lastMeasureOffValue, thresholdHigh, thresholdLow;
    bool wasLow, wasHigh = false;
    unsigned long pulseCounter = 0;
    RunningMedian samples = RunningMedian(3);

    void onValue(uint16_t value);

    Action nextAction = MEASURE_OFF;
    unsigned long nextActionAt = 0;

  #if DEBUGGING
    void resetDebugging();
    uint16_t debugHighestValue, debugLowestValue = 0;
  #endif
};
