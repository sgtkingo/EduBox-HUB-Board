/**
 * @file main.cpp
 * @brief EduBox HUB firmware bootstrap and unified device registry.
 *
 * The application exposes the same handler-based VSCP server over USB Serial
 * and UART2. Every physical sensor and actuator is registered as Device, while
 * its Sxx/Axx UID remains stable for existing HMI device databases.
 */

#include "BoardConfig.hpp"
#include "UartDebugger.hpp"

#include <HardwareSerial.h>
#include <devices.hpp>
#include <engine.hpp>
#include <esp_log.h>
#include <vscp.hpp>
#if EDUBOX_BLE_ENABLED
#include <edubox_ble.hpp>
#endif
#include <mbedtls/md.h>
#include <cstdio>
#include <cstring>

String makeBleBoardId() {
  const uint64_t identity = ESP.getEfuseMac() & 0x0000FFFFFFFFFFFFULL;
  char value[17];
  std::snprintf(value, sizeof(value), "EB-%04lX-%08lX",
      static_cast<unsigned long>((identity >> 32) & 0xFFFFULL),
      static_cast<unsigned long>(identity & 0xFFFFFFFFULL));
  return String(value);
}

uint32_t makeBlePairingPin(const String& boardId) {
  const auto* algorithm = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  unsigned char digest[32]{};
  if (!algorithm || mbedtls_md_hmac(algorithm,
      reinterpret_cast<const unsigned char*>(blePairingKey), std::strlen(blePairingKey),
      reinterpret_cast<const unsigned char*>(boardId.c_str()), boardId.length(), digest) != 0) {
    return 0;
  }
  const uint32_t value = (uint32_t(digest[0]) << 24) | (uint32_t(digest[1]) << 16) |
      (uint32_t(digest[2]) << 8) | uint32_t(digest[3]);
  return 100000U + (value % 900000U);
}

String formatBlePin(uint32_t pin) {
  char value[7];
  std::snprintf(value, sizeof(value), "%06lu", static_cast<unsigned long>(pin));
  return String(value);
}

namespace {

#if EDUBOX_BLE_ENABLED
class DebugBleTransport : public edubox::ble::Transport {
public:
  DebugBleTransport(edubox::ble::Channel& channel, UartDebugger& debugger)
      : edubox::ble::Transport(channel), protocolDebugger_(debugger, "BT") {}

protected:
  vscp::ReadStatus readLineImpl(String& message) override {
    const auto status = edubox::ble::Transport::readLineImpl(message);
    if (status == vscp::ReadStatus::Message) protocolDebugger_.received(message);
    else if (status == vscp::ReadStatus::MessageTooLong)
      protocolDebugger_.readError("request too long");
    return status;
  }

  bool writeLineImpl(const String& message) override {
    const bool written = edubox::ble::Transport::writeLineImpl(message);
    protocolDebugger_.sent(message, written);
    return written;
  }

private:
  ProtocolDebugger protocolDebugger_;
};
#endif

RegisteredDevice registeredDevices[] = {
  {"S00", new DS18B20()},
  {"S01", new DHT11x(terminal2Pin)},
  {"S02", new SensorDigitalRead(terminal2Pin, 2, "Dhall")},
  {"S03", new Ahall(terminal2Pin)},
  {"S04", new SensorDigitalRead(terminal1Pin, 4, "PInterrupt")},
  {"S05", new SensorDigitalRead(terminal1Pin, 5, "FC51")},
  {"S06", new HCSR04(terminal1Pin, terminal2Pin)},
  {"S07", new SensorDigitalRead(terminal1Pin, 7, "HCSR501")},
  {"S08", new SensorDigitalRead(terminal1Pin, 8, "KW113Z")},
  {"S09", new BMP280(sensorSdaPin, sensorSclPin)},
  {"S10", new BMP180(sensorSdaPin, sensorSclPin)},
  {"S11", new TCS34725(sensorSdaPin, sensorSclPin)},
  {"S12", new IRrx(terminal1Pin)},
  {"S13", new SensorDigitalRead(terminal1Pin, 13, "Dntc")},
  {"S14", new Antc(terminal2Pin)},
  {"S15", new PHresistance(terminal2Pin)},
  {"S16", new Joystick(joystickXPin, joystickYPin, joystickSwitchPin)},
  {"S17", new HallLin(terminal2Pin)},
  {"S18", new SensorDigitalRead(terminal1Pin, 18, "MQ135")},
  {"S19", new SensorDigitalRead(terminal1Pin, 19, "DMoisture")},
  {"S20", new SensorDigitalRead(terminal1Pin, 20, "TTP223")},
  {"S21", new GP2Y0A21YK0F(terminal1Pin)},
  {"S22", new Rencoder(terminal3Pin, terminal4Pin)},
  {"S23", new SensorDigitalRead(terminal1Pin, 23, "HS0038DB")},
  {"S24", new SensorDigitalRead(terminal1Pin, 24, "TCRT5000")},
  {"S25", new SensorDigitalRead(terminal1Pin, 25, "IRflame")},
  {"S26", new SensorDigitalRead(terminal2Pin, 26, "REED")},
  {"S27", new MicSmall(terminal1Pin, microphoneSampleWindowMs)},
  {"S28", new MicBig(terminal1Pin, microphoneSampleWindowMs)},
  {"S29", new SensorDigitalRead(terminal1Pin, 29, "MetalTouch")},
  {"S30", new Heartbeat(terminal2Pin, 5000)},
  {"S31", new SensorDigitalRead(terminal2Pin, 31, "Btn")},
  {"S32", new SensorDigitalRead(terminal2Pin, 32, "TiltSwitch")},
  {"S33", new SensorDigitalRead(terminal2Pin, 33, "Dvibration")},
  {"S34", new SensorDigitalRead(terminal2Pin, 34, "HGswitch")},
  {"S35", new SensorDigitalRead(terminal2Pin, 35, "Tap")},

  // Actuators own no GPIO and produce no output before CONNECT + CONTROL.
  {"A00", new SG90(-1, 0, 100)},
  {"A01", new Stepper(-1, -1, -1, -1, 0, true, 16)},
  {"A02", new DC(-1, 50, false)},
  {"A03", new TwoColor(-1, -1, 'R', 0)},
  {"A04", new TwoColorMini(-1, -1, 'G', 0)},
  {"A05", new RGB()},
  {"A06", new RGB()},
  {"A07", new Color7(-1, false)},
  {"A08", new IRtx(-1, 0)},
  {"A09", new Laser(-1, false)},
  {"A10", new BuzzP(-1, 1000, 500)},
  {"A11", new BuzzA(-1, false)}
};

constexpr size_t registeredDeviceCount = sizeof(registeredDevices) / sizeof(registeredDevices[0]);

HardwareSerial protocolSerial(vscpUartConfig.port);
#if ARDUINO_USB_CDC_ON_BOOT
UartDebugger uartDebugger(Serial0);
#else
UartDebugger uartDebugger(Serial);  // Serial is UART0 when USB CDC is disabled.
#endif
DebugStreamTransport usbTransport(Serial, uartDebugger, "USB");
DebugStreamTransport uartTransport(protocolSerial, uartDebugger, "UART2");
vscp::Server protocolServer;
VscpDeviceRouter deviceRouter(registeredDevices, registeredDeviceCount,
    vscpControlLeaseMs, vscpControlProbeIntervalMs, vscpControlProbeTimeoutMs);
#if EDUBOX_BLE_ENABLED
edubox::ble::Channel bleChannel;
DebugBleTransport bleTransport(bleChannel, uartDebugger);
edubox::ble::Peripheral bleBridge(bleChannel);
uint32_t bleButtonAt = 0;
String bleBoardId;
uint32_t bleCommissioningPin = 0;
bool bleButtonHeld = false;
bool bleWasOnline = false;
edubox::ble::Stats bleLastStats;
#endif

}  // namespace

void setup() {
  Serial.begin(usbProtocolBaudRate);
#if ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(usbProtocolBaudRate);
#endif
  uartDebugger.log("DEBUG", "Board initialization started");
  esp_log_level_set("*", uartDebugEnabled ? ESP_LOG_WARN : ESP_LOG_ERROR);

  protocolSerial.begin(
      vscpUartConfig.baudRate,
      vscpUartConfig.frameFormat,
      vscpUartConfig.rxPin,
      vscpUartConfig.txPin);
  uartDebugger.log("DEBUG", String("UART") + vscpUartConfig.port + " initialized");

  deviceRouter.registerHandlers(protocolServer);
  deviceRouter.setReservedPins({vscpUartConfig.rxPin, vscpUartConfig.txPin, 43, 44
#if ARDUINO_USB_CDC_ON_BOOT
      , 19, 20
#endif
  });
  if (usbProtocolEnabled) protocolServer.addTransport(usbTransport);
  protocolServer.addTransport(uartTransport);
#if EDUBOX_BLE_ENABLED
  pinMode(bleResetButtonPin, INPUT_PULLUP);
  deviceRouter.setTransportAvailabilityCheck([](const vscp::Transport& transport) {
    return &transport != &bleTransport || bleChannel.online();
  });
  // GPIO0 is reserved for local bond reset, not a remotely assignable output.
  deviceRouter.setReservedPins({vscpUartConfig.rxPin, vscpUartConfig.txPin, 43, 44,
      bleResetButtonPin
#if ARDUINO_USB_CDC_ON_BOOT
      , 19, 20
#endif
  });
  bleBoardId = makeBleBoardId();
  bleCommissioningPin = makeBlePairingPin(bleBoardId);
  uartDebugger.log("DEBUG", String("BT bridge initialization started board_id=") + bleBoardId +
      " pairing_window_ms=" + blePairingWindowMs);
  if (bleCommissioningPin != 0 &&
      bleBridge.begin(bleBoardId.c_str(), bleCommissioningPin, false, blePairingWindowMs)) {
    protocolServer.on(vscp::Command::Pair, [](const vscp::Request& request, vscp::Transport& source) {
      if (&source != &uartTransport) return vscp::Response::fail("PAIR requires Board UART");
      const bool reset = request.has("reset") && request.value("reset") == "1";
      if (request.has("reset") && !reset) return vscp::Response::fail("Invalid reset");
      if (bleBridge.paired() && !reset) return vscp::Response::fail("already_paired");
      if (reset) {
        deviceRouter.notifyTransportDisconnected(bleTransport);
        if (!bleBridge.forgetBond(blePairingWindowMs))
          return vscp::Response::fail("pairing_reset_failed");
      } else if (!bleBridge.openPairingWindow(blePairingWindowMs)) {
        return vscp::Response::fail(bleBridge.paired() ? "already_paired" : "pairing_unavailable");
      }
      auto response = vscp::Response::ok();
      response.parameters["board_id"] = bleBoardId;
      response.parameters["pin"] = formatBlePin(bleCommissioningPin);
      return response;
    });
    protocolServer.addTransport(bleTransport);
    bleLastStats = bleChannel.stats();
    uartDebugger.log("DEBUG", String("BT bridge initialized board_id=") + bleBoardId +
        " advertising=1");
    uartDebugger.log("INFO", String("BT Board ID=") + bleBoardId +
        " PIN=" + formatBlePin(bleCommissioningPin) +
        " pairing_window_ms=" + blePairingWindowMs);
  } else {
    uartDebugger.log("ERROR", "BT bridge initialization failed; UART remains available");
  }
#endif
  uartDebugger.log("INFO", String("VSCP UART") + vscpUartConfig.port +
                              " RX=" + vscpUartConfig.rxPin +
                              " TX=" + vscpUartConfig.txPin +
                              " baud=" + vscpUartConfig.baudRate);
  uartDebugger.log("DEBUG", "Board initialization completed");
}

void loop() {
#if EDUBOX_BLE_ENABLED
  const bool bleLost = bleBridge.poll();
  const bool bleOnline = bleChannel.online();
  if (bleOnline && !bleWasOnline) {
    uartDebugger.log("DEBUG", "BT bridge online; peer authenticated and subscribed");
  }
  if (bleLost) {
    const auto stats = bleChannel.stats();
    if (stats.faults != bleLastStats.faults) {
      uartDebugger.log("ERROR", String("BT bridge fault; releasing control session faults=") +
          stats.faults);
    } else {
      uartDebugger.log("WARN", String("BT bridge disconnected; releasing control session disconnects=") +
          stats.disconnects);
    }
    bleLastStats = stats;
    deviceRouter.notifyTransportDisconnected(bleTransport);
  }
  bleWasOnline = bleOnline;
  if (digitalRead(bleResetButtonPin) == LOW) {
    if (!bleButtonHeld) { bleButtonAt = millis(); bleButtonHeld = true; }
    if (uint32_t(millis() - bleButtonAt) >= 3000) {
      uartDebugger.log("DEBUG", "BT bond reset requested; stopping all transport sessions");
      deviceRouter.notifyTransportDisconnected(bleTransport);
      deviceRouter.notifyTransportDisconnected(uartTransport);
      deviceRouter.notifyTransportDisconnected(usbTransport);
      if (!bleBridge.forgetBond()) {
        uartDebugger.log("ERROR", "BT bond reset failed; outputs stopped; release BOOT to retry");
        bleButtonAt = millis();
        return;
      }
      uartDebugger.log("DEBUG", "BT bond forgotten; restarting for commissioning");
      Serial.flush();
      ESP.restart();
    }
  } else bleButtonHeld = false;
#endif
  deviceRouter.poll();
  protocolServer.poll();
  deviceRouter.poll();
  delay(1); // Yield to NimBLE/FreeRTOS; no transport blocking here.
}
