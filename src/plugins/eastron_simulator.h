#include <ModbusRTU.h>

#define ENABLE_MODBUS_BUSY_RESPONSE false

const uint16_t REGISTER_LINE_TO_NEUTRAL_VOLTS = 0x00;
const uint16_t REGISTER_CURRENT_AMPS = 0x06;
const uint16_t REGISTER_ACTIVE_POWER = 0x0C;
const uint16_t REGISTER_APPARENT_POWER = 0x12;
const uint16_t REGISTER_IMPORT_ACTIVE_ENERGY = 0x48;
const uint16_t REGISTER_EXPORT_ACTIVE_ENERGY = 0x4A;
const uint16_t REGISTER_TOTAL_SYSTEM_POWER_DEMAND = 0x54;
const uint16_t REGISTER_MAXIMUM_TOTAL_SYSTEM_POWER_DEMAND = 0x56;
const uint16_t REGISTER_SYSTEM_POSITIVE_POWER_DEMAND = 0x58;
const uint16_t REGISTER_MAXIMUM_SYSTEM_POSITIVE_POWER_DEMAND = 0x5A;
const uint16_t REGISTER_TOTAL_ACTIVE_ENERGY = 0x156;

class EastronSimulator {
  public:
    EastronSimulator(HardwareSerial *serial) { this->serial = serial; };
    
    void setup();
    void loop();

    void setCurrentPower(float power, float readingIn, float readingOut, float voltage) {
      if (voltage <= 0) { voltage = 230; } // Fallback in case voltage is not initialzed properly.

      power = roundf(power * 10) / 10;

      setRegisterValue(REGISTER_LINE_TO_NEUTRAL_VOLTS, roundf(voltage * 10) / 10);
      setRegisterValue(REGISTER_CURRENT_AMPS, roundf(power * 10 / voltage) / 10); // Approximate current ampere
      setRegisterValue(REGISTER_ACTIVE_POWER, power);
      setRegisterValue(REGISTER_APPARENT_POWER, power);

      setRegisterValue(REGISTER_IMPORT_ACTIVE_ENERGY, readingIn);
      setRegisterValue(REGISTER_EXPORT_ACTIVE_ENERGY, readingOut);

      setRegisterValue(REGISTER_TOTAL_SYSTEM_POWER_DEMAND, power);
      setRegisterValue(REGISTER_MAXIMUM_TOTAL_SYSTEM_POWER_DEMAND, power);
      setRegisterValue(REGISTER_SYSTEM_POSITIVE_POWER_DEMAND, power);
      setRegisterValue(REGISTER_MAXIMUM_SYSTEM_POSITIVE_POWER_DEMAND, power);

      setRegisterValue(REGISTER_TOTAL_ACTIVE_ENERGY, readingIn + readingOut);

      hasUpdatedData = true;
    }

  private:
    HardwareSerial *serial;
    ModbusRTU modbus;
    bool hasUpdatedData = false;

    void addRegister(uint16_t offset) {
      addRegister(offset, 0);
    }

    void addRegister(uint16_t offset, float value) {
      modbus.addIreg(offset);
      modbus.addIreg(offset + 1);

      setRegisterValue(offset, value);
    }

    void setRegisterValue(uint16_t offset, float value) {
      uint16_t *src = (uint16_t*)&value;

      // The mobdbus library will switch endiness on individual uint16_t value.
      // We still need to "reverse" the register values though.
      modbus.Ireg(offset, src[1]);
      modbus.Ireg(offset + 1, src[0]);
    }
};
