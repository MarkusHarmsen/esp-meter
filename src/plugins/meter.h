#include <PubSubClient.h>
#pragma once

#define HA_SENSOR_PREFIX "homeassistant/sensor/meter_esp/"

class Meter {
  public:
    Meter(PubSubClient *client) { this->client = client; }
    void publishHomeAssistantAutoDiscovery();
    void publishNow();
    void setup();
    void loop();
    bool hasChangesToBePublished();

  protected:
    PubSubClient *client = NULL;
    bool dirty = false;

    void setDirty() {
      this->dirty = true;
    }

    void didPublish() {
      this->dirty = false;
    }

    void publishAutoDiscovery(const char* topic, const char* payload) {
      this->client->publish(topic, payload, true);
    }

    void publishFloat(const char* topic, float value) {
      char buffer[16];
      sprintf(buffer, "%.3f", value);
      this->client->publish(topic, buffer);
    }

    void publishCounter(const char* topic, long unsigned value) {
      char buffer[16];
      sprintf(buffer, "%lu", value);
      this->client->publish(topic, buffer);
    }

    void publishString(const char* topic, const char* payload) {
      this->client->publish(topic, payload);
    }
};
