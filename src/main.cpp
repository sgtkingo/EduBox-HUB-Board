/**
 * @file main.cpp
 * @brief EduBox HUB firmware bootstrap and unified device registry.
 *
 * The application exposes the same handler-based VSCP server over USB Serial
 * and UART2. Every physical sensor and actuator is registered as Device, while
 * its Sxx/Axx UID remains stable for existing HMI device databases.
 */

#include "libs.hpp"
#include "BoardConfig.hpp"
#include "VscpDeviceRouter.hpp"

#include <HardwareSerial.h>
#include <esp_log.h>
#include <vscp.hpp>

namespace {

constexpr uint32_t VSCP_BAUD_RATE = 115200;
constexpr int VSCP_RX_PIN = 18;
constexpr int VSCP_TX_PIN = 17;

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

  {"A00", new SG90(terminal1Pin, 42, 100)},
  {"A01", new Stepper(terminal1Pin, terminal2Pin, terminal3Pin, terminal4Pin, 42, true, 16)},
  {"A02", new DC(terminal1Pin, 50, true)},
  {"A03", new TwoColor(terminal3Pin, terminal4Pin, 'R', 50)},
  {"A04", new TwoColorMini(terminal4Pin, terminal3Pin, 'G', 100)},
  {"A05", new RGB(terminal1Pin, terminal2Pin, terminal3Pin, 0, 0, 50)},
  {"A06", new RGB(terminal1Pin, terminal2Pin, terminal3Pin, 0, 0, 50)},
  {"A07", new Color7(terminal1Pin, true)},
  {"A08", new IRtx(terminal2Pin, 0)},
  {"A09", new Laser(terminal2Pin, true)},
  {"A10", new BuzzP(terminal1Pin, 1000, 500)},
  {"A11", new BuzzA(terminal1Pin, true)}
};

constexpr size_t registeredDeviceCount = sizeof(registeredDevices) / sizeof(registeredDevices[0]);

HardwareSerial protocolSerial(2);
vscp::StreamTransport usbTransport(Serial);
vscp::StreamTransport uartTransport(protocolSerial);
vscp::Server protocolServer;
VscpDeviceRouter deviceRouter(registeredDevices, registeredDeviceCount);

}  // namespace

void setup() {
  Serial.begin(VSCP_BAUD_RATE);
  esp_log_level_set("*", ESP_LOG_ERROR);
  esp_log_level_set("Wire", ESP_LOG_NONE);

  protocolSerial.begin(VSCP_BAUD_RATE, SERIAL_8N1, VSCP_RX_PIN, VSCP_TX_PIN);

  deviceRouter.registerHandlers(protocolServer);
  protocolServer.addTransport(usbTransport);
  protocolServer.addTransport(uartTransport);
}

void loop() {
  protocolServer.poll();
}
