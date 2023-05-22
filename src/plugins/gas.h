#include "meter.h"
#include <Bounce2.h>

#define HA_TOPIC_GAS_COUNTER "meter_esp/gas/counter"
#define HA_TOPIC_GAS_CONSUMPTION_M3 "meter_esp/gas/consumption/m3"
#define HA_TOPIC_GAS_CONSUMPTION_KWH "meter_esp/gas/consumption/kwh"

class GasMeter : Meter {
  public:
    GasMeter(PubSubClient *client, uint8_t pin, float m3ToKWhFactor) : Meter(client) {
      this->pin = pin;
      this->m3ToKWhFactor = m3ToKWhFactor;
    };

    void publishHomeAssistantAutoDiscovery() {
      publishAutoDiscovery(HA_SENSOR_PREFIX "gas_counter/config", "{\"name\": \"Gas Counter\", \"stat_t\": \"" HA_TOPIC_GAS_COUNTER "\", \"obj_id\": \"meter_gas_counter\", \"stat_cla\": \"total_increasing\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "gas_consumption_m3/config", "{\"name\": \"Gas Consumption (m³)\", \"stat_t\": \"" HA_TOPIC_GAS_CONSUMPTION_M3 "\", \"obj_id\": \"meter_gas_consumption_m3\", \"unit_of_meas\": \"m³\", \"stat_cla\": \"total_increasing\", \"dev_cla\": \"gas\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "gas_consumption_kwh/config", "{\"name\": \"Gas Consumption (kWh)\", \"stat_t\": \"" HA_TOPIC_GAS_CONSUMPTION_KWH "\", \"obj_id\": \"meter_gas_consumption_kwh\", \"unit_of_meas\": \"kWh\", \"stat_cla\": \"total_increasing\", \"dev_cla\": \"energy\" }");
    };

    void publishNow();
    void setup();
    void loop();
    bool hasChangesToBePublished() { return dirty; }

  private:
    uint8_t pin;
    float m3ToKWhFactor;    
    Bounce bounce;
    unsigned long pulseCounter = 0;
    unsigned long nextActionAt = 0;
};
