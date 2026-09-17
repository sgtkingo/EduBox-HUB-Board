#include <VscpDeviceRouter.hpp>
#include <DC.hpp>
#include <SG90.hpp>
#include <Stepper.hpp>
#include <BuzzP.hpp>
#include <7color.hpp>
#include <IRtx.hpp>
#include <Laser.hpp>
#include <cassert>
#include <deque>
#include <iostream>

static uint32_t nowMs = 0;
static uint32_t clockMs() { return nowMs; }
static int irTransmissions = 0;
void IRtx_control(int, uint32_t) { ++irTransmissions; }
void IRtx_reset(int) {}
void Color7_control(int pin, bool enabled) { digitalWrite(pin, enabled ? HIGH : LOW); }
void Color7_reset(int pin) { digitalWrite(pin, LOW); }
static int laserPin = -1;
void Laser_setPin(int pin) { laserPin = pin; }
void Laser_control(bool enabled) { if (laserPin >= 0) digitalWrite(laserPin, enabled ? HIGH : LOW); }
void Laser_reset() { Laser_control(false); }

class Wire : public vscp::Transport {
public:
  std::deque<String> incoming;
  std::vector<String> outgoing;
  bool writable = true;
protected:
  vscp::ReadStatus readLineImpl(String& message) override {
    if (incoming.empty()) return vscp::ReadStatus::NoData;
    message = incoming.front(); incoming.pop_front(); return vscp::ReadStatus::Message;
  }
  bool writeLineImpl(const String& message) override {
    if (!writable) return false;
    outgoing.push_back(message); return true;
  }
};

class Sensor : public Device {
public:
  int detachCalls = 0, resetCalls = 0;
  DeviceType deviceType() const override { return DeviceType::DigitalInput; }
  void attach(const std::vector<int>& pins) override { pin_ = pins[0]; pinMode(pin_, INPUT); }
  void detach() override { ++detachCalls; pinMode(pin_, INPUT); }
  void reset() override { ++resetCalls; }
private:
  int pin_ = -1;
};

static vscp::ResponseStatus request(vscp::Server& server, Wire& wire, const String& line) {
  wire.incoming.push_back(line);
  server.poll();
  vscp::ResponseStatus response;
  String error;
  assert(!wire.outgoing.empty() && vscp::Codec::parseResponse(wire.outgoing.back(), response, error));
  return response;
}
static void init(vscp::Server& server, Wire& wire) {
  assert(request(server, wire, "?type=INIT&api=1.6").status == vscp::Status::Ok);
}

int main() {
  DC motor(-1, 50, true);
  Sensor sensor;
  RegisteredDevice devices[] = {{"A02", &motor}, {"S01", &sensor}};
  Wire usb, uart;
  vscp::Server server;
  server.addTransport(usb); server.addTransport(uart);
  VscpDeviceRouter router(devices, 2, 10000, 3000, 500, clockMs);
  router.registerHandlers(server); router.setReservedPins({17, 18});

  // An incompatible INIT must not claim the Board.
  assert(request(server, usb, "?type=INIT&api=1.4").status == vscp::Status::Error);
  init(server, uart);
  assert(request(server, uart, "?type=CONNECT&id=A02&pins=15").status == vscp::Status::Ok);
  assert(pinValues[15] == LOW); // No startup PWM even with constructor state=true.
  assert(request(server, uart, "?type=CONTROL&id=A02&state=1&speed=75").status == vscp::Status::Ok);
  const int runningPwm = pinValues[15]; assert(runningPwm > 0);
  assert(request(server, usb, "?type=INIT&api=1.6").status == vscp::Status::Error);
  assert(request(server, usb, "?type=CONTROL&id=A02&state=0").status == vscp::Status::Error);
  assert(request(server, usb, "?type=RESET&id=A*").status == vscp::Status::Error);
  assert(pinValues[15] == runningPwm);

  // Non-owner PING/BYE must never take over or stop the owner's hardware.
  assert(request(server, usb, "?type=PING&side=client&seq=1").status == vscp::Status::Ok);
  usb.incoming.push_back("?type=BYE&side=client"); server.poll();
  assert(pinValues[15] == runningPwm && devices[0].connected);
  router.notifyTransportDisconnected(usb);
  assert(pinValues[15] == runningPwm);

  // Validate assignments before touching a currently running motor.
  assert(request(server, uart, "?type=CONNECT&id=S01&pins=7").status == vscp::Status::Ok);
  for (const auto* pins : {"7", "17", "999"}) {
    assert(request(server, uart, String("?type=CONNECT&id=A02&pins=") + pins).status == vscp::Status::Error);
    assert(pinValues[15] == runningPwm);
  }

  // An owner BYE stops outputs, releases pins/session records, and permits a new owner.
  uart.incoming.push_back("?type=BYE&side=client"); server.poll();
  assert(pinValues[15] == LOW && pinModes[15] == OUTPUT);
  assert(!devices[0].connected && !devices[1].connected && devices[0].pins.empty());
  assert(sensor.detachCalls == 1 && sensor.resetCalls == 0);
  uart.incoming.push_back("?type=BYE&side=client"); server.poll();
  assert(sensor.detachCalls == 1);
  init(server, usb);
  assert(request(server, uart, "?type=CONTROL&id=A02&state=1").status == vscp::Status::Error);
  assert(request(server, usb, "?type=RESET&id=A02").status == vscp::Status::Error);
  assert(request(server, usb, "?type=CONNECT&id=A02&pins=15").status == vscp::Status::Ok);
  assert(pinValues[15] == LOW); // Reconnect cannot restore the old enabled state.

  // Idle clients keep their lease by replying to Board-initiated PING.
  nowMs = 3000; router.poll();
  vscp::Request ping; String error;
  assert(vscp::Codec::parseRequest(usb.outgoing.back(), ping, error));
  assert(ping.command == vscp::Command::Ping);
  usb.incoming.push_back(String("?side=client&status=1&seq=") + ping.value("seq"));
  server.poll(); nowMs = 3001; router.poll();
  usb.writable = false;
  nowMs = 13000; router.poll(); assert(devices[0].connected);
  nowMs = 13001; router.poll(); assert(!devices[0].connected);
  assert(pinValues[15] == LOW); // Even a failed BYE write cannot prevent shutdown.
  usb.writable = true; init(server, uart);

  // A physical link-down notification also releases a silent owner immediately.
  assert(request(server, uart, "?type=CONNECT&id=A02&pins=15").status == vscp::Status::Ok);
  request(server, uart, "?type=CONTROL&id=A02&state=1");
  router.notifyTransportDisconnected(uart);
  assert(!devices[0].connected && pinValues[15] == LOW);
  assert(request(server, uart, "?type=CONTROL&id=A02&state=1").status == vscp::Status::Error);
  init(server, usb);

  // millis rollover must not cause premature expiry or an immortal lease.
  router.notifyTransportDisconnected(usb);
  nowMs = UINT32_MAX - 1500; init(server, uart);
  nowMs = 1000; router.poll();
  assert(request(server, usb, "?type=INIT&api=1.6").status == vscp::Status::Error);
  nowMs = 9000; router.poll(); init(server, usb);

  // Real actuator lifecycle code: CONNECT silent; detach never runs RESET movement.
  SG90 servo(-1, 90, 100);
  servo.attach({4}); assert(servo.init());
  assert(servoStarts == 0 && servoWrites == 0);
  Param servoCommand[] = {{"angle", "45"}}; servo.control(servoCommand, 1);
  assert(servoStarts == 1 && servoWrites > 0);
  const int writesBeforeDetach = servoWrites;
  servo.detach(); assert(servoWrites == writesBeforeDetach);
  BuzzP buzzer(-1, 1000, 500); buzzer.attach({5}); assert(buzzer.init());
  assert(toneStarts == 0); buzzer.control(); assert(toneStarts == 1);
  buzzer.detach(); assert(pinValues[5] == LOW);
  Color7 led(-1, true); led.attach({6}); assert(pinValues[6] == LOW);
  IRtx ir(-1, 123); ir.attach({8}); assert(irTransmissions == 0);
  ir.control(); assert(irTransmissions == 1);
  Laser laser(-1, true); laser.attach({9}); assert(pinValues[9] == LOW);
  Stepper stepper(-1, -1, -1, -1, 90, true, 16);
  stepper.attach({10, 11, 12, 13}); assert(stepper.init());
  for (const int pin : {10, 11, 12, 13}) pinValues[pin] = HIGH;
  stepper.detach();
  for (const int pin : {10, 11, 12, 13}) assert(pinValues[pin] == LOW);

  // Long motions return promptly, so BYE can cancel them before completion.
  RegisteredDevice motionDevices[] = {{"A00", &servo}, {"A01", &stepper}};
  Wire motionWire;
  vscp::Server motionServer; motionServer.addTransport(motionWire);
  VscpDeviceRouter motionRouter(motionDevices, 2, 10000, 3000, 500, clockMs);
  motionRouter.registerHandlers(motionServer); init(motionServer, motionWire);
  request(motionServer, motionWire, "?type=CONNECT&id=A00&pins=4");
  request(motionServer, motionWire, "?type=CONNECT&id=A01&pins=10,11,12,13");
  const auto started = std::chrono::steady_clock::now();
  request(motionServer, motionWire, "?type=CONTROL&id=A00&angle=180&speed=0");
  request(motionServer, motionWire, "?type=CONTROL&id=A01&angle=180&dir=1");
  assert(std::chrono::steady_clock::now() - started < std::chrono::milliseconds(100));
  motionRouter.poll();
  const int servoWritesWhileMoving = servoWrites;
  motionWire.incoming.push_back("?type=BYE&side=client"); motionServer.poll();
  assert(!motionDevices[0].connected && !motionDevices[1].connected);
  assert(servoWrites == servoWritesWhileMoving); // Stop does not move to zero.
  motionRouter.poll(); assert(servoWrites == servoWritesWhileMoving);
  for (const int pin : {4, 10, 11, 12, 13}) assert(pinValues[pin] == LOW && pinModes[pin] == OUTPUT);

  // A stale queued CONTROL cannot renew an expired lease before the next loop poll.
  init(motionServer, motionWire);
  request(motionServer, motionWire, "?type=CONNECT&id=A00&pins=4");
  const int startsBeforeStaleCommand = servoStarts;
  nowMs += 10000;
  motionWire.incoming.push_back("?type=CONTROL&id=A00&angle=90");
  motionServer.poll();
  assert(!motionDevices[0].connected && servoStarts == startsBeforeStaleCommand);

  // Rejected reinitialization by the current owner must not leave active outputs.
  init(motionServer, motionWire);
  request(motionServer, motionWire, "?type=CONNECT&id=A00&pins=4");
  request(motionServer, motionWire, "?type=CONTROL&id=A00&angle=45&speed=100");
  assert(request(motionServer, motionWire, "?type=INIT&api=1.4").status == vscp::Status::Error);
  assert(!motionDevices[0].connected && pinValues[4] == LOW);
  std::cout << "PASS ownership, lease, heartbeat, link loss, GPIO protection and actuator lifecycle\n";
}
