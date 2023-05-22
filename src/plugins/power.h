#include "meter.h"
#include <RunningMedian.h>

const int BUFFER_SIZE = 40; // Longest expected message: 34 bytes with +2 due to "\n\r"

#define HA_TOPIC_POWER_READINGS_IN "meter_esp/power/readings_in"
#define HA_TOPIC_POWER_READINGS_OUT "meter_esp/power/readings_out"
#define HA_TOPIC_POWER_CURRENT "meter_esp/power/current"
#define HA_TOPIC_POWER_PHASE_1 "meter_esp/power/phase_1"
#define HA_TOPIC_POWER_PHASE_2 "meter_esp/power/phase_2"
#define HA_TOPIC_POWER_PHASE_3 "meter_esp/power/phase_3"
#define HA_TOPIC_POWER_VOLTAGE_PHASE_1 "meter_esp/power/voltage_phase_1"
#define HA_TOPIC_POWER_VOLTAGE_PHASE_2 "meter_esp/power/voltage_phase_2"
#define HA_TOPIC_POWER_VOLTAGE_PHASE_3 "meter_esp/power/voltage_phase_3"

class PowerMeter: public Meter {
  public:
    PowerMeter(PubSubClient *client, HardwareSerial *serial) : Meter(client) {
      this->serial = serial;
    };

    void publishHomeAssistantAutoDiscovery() {
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_readings_in/config", "{\"name\": \"Power ReadingsIn\", \"stat_t\": \"" HA_TOPIC_POWER_READINGS_IN "\", \"obj_id\": \"meter_power_readings_in\", \"unit_of_meas\": \"kWh\", \"stat_cla\": \"total\", \"dev_cla\": \"energy\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_readings_out/config", "{\"name\": \"Power ReadingsOut\", \"stat_t\": \"" HA_TOPIC_POWER_READINGS_OUT "\", \"obj_id\": \"meter_power_readings_out\", \"unit_of_meas\": \"kWh\", \"stat_cla\": \"total\", \"dev_cla\": \"energy\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_current/config", "{\"name\": \"Power Current\", \"stat_t\": \"" HA_TOPIC_POWER_CURRENT "\", \"obj_id\": \"meter_power_current\", \"unit_of_meas\": \"W\", \"stat_cla\": \"measurement\", \"dev_cla\": \"power\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_current_phase_1/config", "{\"name\": \"Power Phase 1\", \"stat_t\": \"" HA_TOPIC_POWER_PHASE_1 "\", \"obj_id\": \"meter_power_phase_1\", \"unit_of_meas\": \"W\", \"stat_cla\": \"measurement\", \"dev_cla\": \"power\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_current_phase_2/config", "{\"name\": \"Power Phase 2\", \"stat_t\": \"" HA_TOPIC_POWER_PHASE_2 "\", \"obj_id\": \"meter_power_phase_2\", \"unit_of_meas\": \"W\", \"stat_cla\": \"measurement\", \"dev_cla\": \"power\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_current_phase_3/config", "{\"name\": \"Power Phase 3\", \"stat_t\": \"" HA_TOPIC_POWER_PHASE_3 "\", \"obj_id\": \"meter_power_phase_3\", \"unit_of_meas\": \"W\", \"stat_cla\": \"measurement\", \"dev_cla\": \"power\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_current_voltage_phase_1/config", "{\"name\": \"Voltage Phase 1\", \"stat_t\": \"" HA_TOPIC_POWER_VOLTAGE_PHASE_1 "\", \"obj_id\": \"meter_power_voltage_phase_1\", \"unit_of_meas\": \"V\", \"stat_cla\": \"measurement\", \"dev_cla\": \"voltage\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_current_voltage_phase_2/config", "{\"name\": \"Voltage Phase 2\", \"stat_t\": \"" HA_TOPIC_POWER_VOLTAGE_PHASE_2 "\", \"obj_id\": \"meter_power_voltage_phase_2\", \"unit_of_meas\": \"V\", \"stat_cla\": \"measurement\", \"dev_cla\": \"voltage\" }");
      publishAutoDiscovery(HA_SENSOR_PREFIX "power_current_voltage_phase_3/config", "{\"name\": \"Voltage Phase 3\", \"stat_t\": \"" HA_TOPIC_POWER_VOLTAGE_PHASE_3 "\", \"obj_id\": \"meter_power_voltage_phase_3\", \"unit_of_meas\": \"V\", \"stat_cla\": \"measurement\", \"dev_cla\": \"voltage\" }");
    };

    void publishNow();
    void setup();
    void loop();
    bool hasChangesToBePublished() { return dirty; }

    bool isOnline() {
      // We should be online if our samples are full (and readings not negative)
      return readingsIn >= 0 && readingsOut >= 0 && samplesReadingsIn.isFull() && samplesReadingsOut.isFull();
    }

    float getReadingsInStable() {
      return samplesReadingsIn.getMedian();
    }

    float getReadingsOutStable() {
      return samplesReadingsOut.getMedian();
    }

    float getCurrentPower() {
      return currentPower;
    }

    float getCurrentVoltage() {
      return (currentVoltagePhase1 + currentVoltagePhase2 + currentVoltagePhase3) / 3;
    }

    bool currentPowerUpdated = false;

  private:
    HardwareSerial *serial;

    char buffer[BUFFER_SIZE] = "";
    int bufferIndex = 0;
    bool skipToNextMessage = false;

    float readingsIn = -1;
    float readingsOut = -1;
    float currentPower = 0;
    float currentPowerPhase1 = 0;
    float currentPowerPhase2 = 0;
    float currentPowerPhase3 = 0;
    float currentVoltagePhase1 = 0;
    float currentVoltagePhase2 = 0;
    float currentVoltagePhase3 = 0;
    unsigned long seconds = 0;

    const char *UNIT_KWH = "kWh";
    const char *UNIT_W = "W";
    const char *UNIT_V = "V";

    RunningMedian samplesReadingsIn = RunningMedian(5);
    RunningMedian samplesReadingsOut = RunningMedian(5);

    void readByte(char byte);
    void receivedMessage();
    void clearBuffer();
    bool parseObisFloatFromBuffer(char *sourceBuffer, float &target, uint8_t preDecimalDigits, uint8_t decimalDigits, bool allowNegative, const char *unit);
};
