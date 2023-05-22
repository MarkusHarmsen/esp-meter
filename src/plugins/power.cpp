#include "power.h"

void PowerMeter::publishNow() {
  if (!isOnline()) { return; }

  // For readings in/out its more important to get an accurate than fast value.
  // We therefore apply the median from the last 5 values (e.g. 5 seconds) as "filter" to catch outliers.
  publishFloat(HA_TOPIC_POWER_READINGS_IN, getReadingsInStable());
  publishFloat(HA_TOPIC_POWER_READINGS_OUT, getReadingsOutStable());

  // The current values can change fast if load changes
  publishFloat(HA_TOPIC_POWER_CURRENT, currentPower);
  publishFloat(HA_TOPIC_POWER_PHASE_1, currentPowerPhase1);
  publishFloat(HA_TOPIC_POWER_PHASE_2, currentPowerPhase2);
  publishFloat(HA_TOPIC_POWER_PHASE_3, currentPowerPhase3);
  if (currentVoltagePhase1 > 0 || currentVoltagePhase2 > 0 || currentVoltagePhase3 > 0) {
    publishFloat(HA_TOPIC_POWER_VOLTAGE_PHASE_1, currentVoltagePhase1);
    publishFloat(HA_TOPIC_POWER_VOLTAGE_PHASE_2, currentVoltagePhase2);
    publishFloat(HA_TOPIC_POWER_VOLTAGE_PHASE_3, currentVoltagePhase3);
  }

  didPublish();
}

void PowerMeter::setup() {
  serial->begin(9600, SERIAL_7E1);
  skipToNextMessage = true; // Initial message is very likely to be missed, so skip first
}

void PowerMeter::loop() {
  while (serial->available() > 0) {
    readByte(serial->read());
  }
}

void PowerMeter::readByte(char byte) {
  if (byte == '\r') { return; } // Ignore carrier return

  if (byte == '\n') {
    // Easy parsing: do not write the newline, instead end the string
    buffer[bufferIndex] = 0x00;

    // Handle message (if there is any)
    if (!skipToNextMessage && bufferIndex > 0) { receivedMessage(); }

    skipToNextMessage = false;
    clearBuffer();
    return;
  }

  if (!skipToNextMessage) {
    buffer[bufferIndex] = byte;
    bufferIndex++;
  }

  // Handle buffer overflow (when we missed a newline)
  if (bufferIndex >= BUFFER_SIZE) {
    skipToNextMessage = true;
    clearBuffer();
  }
}

bool PowerMeter::parseObisFloatFromBuffer(char *sourceBuffer, float &target, uint8_t preDecimalDigits, uint8_t decimalDigits, bool allowNegative, const char *unit) {
  char valueBuffer[preDecimalDigits + decimalDigits + 2] = "";
  int i = 0;

  // Negative sign
  if (*sourceBuffer == '-') {
    if (!allowNegative) { return false; }
    valueBuffer[i++] = *sourceBuffer;
    sourceBuffer++;
  }

  // Digits before the decimal point
  for (int j = 0; j < preDecimalDigits; j++, sourceBuffer++) {
    if (*sourceBuffer < '0' || *sourceBuffer > '9') { return false; }
    valueBuffer[i++] = *sourceBuffer;
  }

  // The decimal point
  if (*sourceBuffer != '.') { return false; }
  valueBuffer[i++] = *sourceBuffer;
  sourceBuffer++;

  // Digits after the decimal point
  for (int j = 0; j < decimalDigits; j++, sourceBuffer++) {
    if (*sourceBuffer < '0' || *sourceBuffer > '9') { return false; }
    valueBuffer[i++] = *sourceBuffer;
  }

  // A star separating the unit
  if (*sourceBuffer != '*') { return false; }
  sourceBuffer++;

  // Unit must match
  if (strncmp(sourceBuffer, unit, strlen(unit)) != 0) { return false; }

  target = atof(valueBuffer);
  return true;
}

void PowerMeter::clearBuffer() {
  bufferIndex = 0;
  memset(buffer, 0, BUFFER_SIZE);
}

void PowerMeter::receivedMessage() {
  // The smart meter opens a package with its "ID" like "/EBZ5DD32R06ETA_106"
  if (*buffer == '/') { return; }

  // The smart meter sends a single "!" at the end of each messages package (all entries send once)
  if (*buffer == '!') { return; }

  auto length = strlen(buffer);

  // Quick check: has to end with ")"
  if (buffer[length - 1] != ')') {
    return;
  }

  // Eigentumsnummer: 1-0:0.0.0*255(1EBZ0100012345)
  if (strncmp(buffer, "1-0:0.0.0*255", 13) == 0) {
    return;
  }

  // Geräte-Identifikation: 1-0:96.1.0*255(1EBZ0100012345)
  if (strncmp(buffer, "1-0:96.1.0*255", 14) == 0) {
    return;
  }

  // Zählerstand zu +A, tariflos: 1-0:1.8.0*255(000054.51235604*kWh)
  if (strncmp(buffer, "1-0:1.8.0*255", 13) == 0) {
    if (parseObisFloatFromBuffer(buffer + 14, readingsIn, 6, 8, false, "kWh")) {
      samplesReadingsIn.add(readingsIn);
      setDirty();
      return;
    }
    return;
  }

  // Zählerstand zu -A, tariflos: 1-0:2.8.0*255(000054.51235604*kWh)
  if (strncmp(buffer, "1-0:2.8.0*255", 13) == 0) {
    if(parseObisFloatFromBuffer(buffer + 14, readingsOut, 6, 8, false, "kWh")) {
      samplesReadingsOut.add(readingsOut);
      setDirty();
      return;
    }
    return;
  }

  // Summe der Momentan-Leistungen in allen Phasen: 1-0:16.7.0*255(000164.30*W)
  // Negative:                                      1-0:16.7.0*255(-000079.54*W)
  if (strncmp(buffer, "1-0:16.7.0*255", 14) == 0) {
    if (parseObisFloatFromBuffer(buffer + 15, currentPower, 6, 2, true, "W")) {
      currentPowerUpdated = true;
      setDirty();
      return;
    }
    return;
  }

  // Momentane Leistung in Phase L1: 1-0:36.7.0*255(000164.30*W)
  if (strncmp(buffer, "1-0:36.7.0*255", 14) == 0) {
    if (parseObisFloatFromBuffer(buffer + 15, currentPowerPhase1, 6, 2, true, "W")) {
      setDirty();
      return;
    }
    return;
  }

  // Momentane Leistung in Phase L2: 1-0:56.7.0*255(000164.30*W)
  if (strncmp(buffer, "1-0:56.7.0*255", 14) == 0) {
    if (parseObisFloatFromBuffer(buffer + 15, currentPowerPhase2, 6, 2, true, "W")) {
      setDirty();
      return;
    }
    return;
  }

  // Momentane Leistung in Phase L3: 1-0:76.7.0*255(000164.30*W)
  if (strncmp(buffer, "1-0:76.7.0*255", 14) == 0) {
    if (parseObisFloatFromBuffer(buffer + 15, currentPowerPhase3, 6, 2, true, "W")) {
      setDirty();
      return;
    }
    return;
  }

  // Spannung in Phase L1: 1-0:32.7.0*255(231.2*V)
  if (strncmp(buffer, "1-0:32.7.0*255", 14) == 0) {
    if (parseObisFloatFromBuffer(buffer + 15, currentVoltagePhase1, 3, 1, false, "V")) {
      setDirty();
      return;
    }
    return;
  }

  // Spannung in Phase L2: 1-0:52.7.0*255(231.2*V)
  if (strncmp(buffer, "1-0:52.7.0*255", 14) == 0) {
    if (parseObisFloatFromBuffer(buffer + 15, currentVoltagePhase2, 3, 1, false, "V")) {
      setDirty();
      return;
    }
    return;
  }
  
  // Spannung in Phase L3: 1-0:72.7.0*255(231.2*V)
  if (strncmp(buffer, "1-0:72.7.0*255", 14) == 0) {
    if (parseObisFloatFromBuffer(buffer + 15, currentVoltagePhase3, 3, 1, false, "V")) {
      setDirty();
      return;
    }
    return;
  }

  // Statuswort: 1-0:96.5.0*255(001C4104)
  if (strncmp(buffer, "1-0:96.5.0*255", 14) == 0 && length == 24) {
    return;
  }

  // Sekundenindex: 0-0:96.8.0*255(09F596C0)
  if (strncmp(buffer, "0-0:96.8.0*255", 14) == 0 && length == 24) {
    seconds = strtol(buffer + 15, NULL, 16);
    setDirty();
    return;
  }

  // Unknown message -> ignore for now
}