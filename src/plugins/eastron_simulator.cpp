#include "eastron_simulator.h"

void EastronSimulator::setup() {
  serial->begin(9600, SERIAL_8N1);
  modbus.begin(serial);
  modbus.server(1);

#if ENABLE_MODBUS_BUSY_RESPONSE
  // Handler which allows to "cancel" a modbus request.
  modbus.onRequest([&](Modbus::FunctionCode fc, const Modbus::RequestData data) -> Modbus::ResultCode {
    if (fc != Modbus::FC_READ_INPUT_REGS) {
      return Modbus::EX_ILLEGAL_FUNCTION;
    }

    // Send timeout when we have no fresh data
    if (data.reg.address == 0x00) {
      if (!hasUpdatedData) { return Modbus::EX_SLAVE_DEVICE_BUSY; }
      hasUpdatedData = false;
    }

    // Proceed as usual
    return Modbus::EX_SUCCESS;
  });
#endif

  /*
    The Growatt asks for the following registers on "slave 1"
      * 0x00  (32 registers)  every 500ms
      * 0x46  (10 registers)  every 1000ms
      * 0x156 (4 registers)   every 1000ms
  */

  // 30001 Line to neutral volts. Volts 00 00
  addRegister(REGISTER_LINE_TO_NEUTRAL_VOLTS);
  // 30007 Current. Amps 00 06
  addRegister(REGISTER_CURRENT_AMPS);
  // 30013 Active power. Watts 00 0C
  addRegister(REGISTER_ACTIVE_POWER);
  // 30019 Apparent power VoltAmps 00 12
  addRegister(REGISTER_APPARENT_POWER);
  // 30025 Reactive power VAr 00 18
  //addRegister(0x18);
  // 30031 Power factor None 00 1E
  addRegister(0x1E, 1);
  // 30037 Phase angle. Degree 00 24
  //addRegister(0x24);
  // 30071 Frequency Hz 00 46
  addRegister(0x46, 50);
  // 30073 Import active energy kwh 00 48
  addRegister(REGISTER_IMPORT_ACTIVE_ENERGY);
  // 30075 Export active energy kwh 00 4A
  addRegister(REGISTER_EXPORT_ACTIVE_ENERGY);
  // 30077 Import reactive energy kvarh 00 4C
  //addRegister(0x4C);
  // 30079 Export reactive energy kvarh 00 4E
  //addRegister(0x4E);
  // 30085 Total system power demand W 00 54
  addRegister(REGISTER_TOTAL_SYSTEM_POWER_DEMAND);
  // 30087 Maximum total system power demand W 00 56
  addRegister(REGISTER_MAXIMUM_TOTAL_SYSTEM_POWER_DEMAND);
  // 30089 Current system positive power demand W 00 58
  addRegister(REGISTER_SYSTEM_POSITIVE_POWER_DEMAND);
  // 30091 Maximum system positive power demand W 00 5A
  addRegister(REGISTER_MAXIMUM_SYSTEM_POSITIVE_POWER_DEMAND);
  // 30093 Current system reverse power demand W 00 5C
  //addRegister(0x5C);
  // 30095 Maximum system reverse power demand W 00 5E
  //addRegister(0x5E);
  // 30259 Current demand. Amps 01 02
  //addRegister(0x0102);
  // 30265 Maximum current demand. Amps 01 08
  //addRegister(0x0108);
  // 30343 Total active energy kwh 01 56
  addRegister(REGISTER_TOTAL_ACTIVE_ENERGY);
  // 30345 Total reactive energy kvarh 01 58
  //addRegister(0x0158);
  // 30385 Current resettable total active energy kwh 01 80
  //addRegister(0x0180);
  // 30387 Current resettable total reactive energy kvarh 01 82
  //addRegister(0x0182);
}

void EastronSimulator::loop() {
  modbus.task();
}
