/**
 * @file VscpDeviceRouter.cpp
 * @brief Implements VSCP command handlers for the EduBox device registry.
 */

#include <VscpDeviceRouter.hpp>
#include <driver/gpio.h>
#include <algorithm>

VscpDeviceRouter::VscpDeviceRouter(RegisteredDevice* devices, size_t deviceCount,
    uint32_t leaseMs, uint32_t probeIntervalMs, uint32_t probeTimeoutMs,
    ExclusiveControlSession::Clock clock)
    : devices_(devices), deviceCount_(deviceCount), leaseMs_(leaseMs),
      probeIntervalMs_(probeIntervalMs), probeTimeoutMs_(probeTimeoutMs), clock_(clock) {}

void VscpDeviceRouter::registerHandlers(vscp::Server& server) {
  auto stop = [this] { stopAllDevices(); };
  if (clock_) session_.reset(new ExclusiveControlSession(server, stop, leaseMs_, probeIntervalMs_, probeTimeoutMs_, clock_));
  else session_.reset(new ExclusiveControlSession(server, stop, leaseMs_, probeIntervalMs_, probeTimeoutMs_));
  server.onBye([this](vscp::Transport& transport) { session_->release(transport); });
  registerHandler(server, vscp::Command::Init, &VscpDeviceRouter::handleInit);
  registerHandler(server, vscp::Command::Connect, &VscpDeviceRouter::handleConnect);
  registerHandler(server, vscp::Command::Disconnect, &VscpDeviceRouter::handleDisconnect);
  registerHandler(server, vscp::Command::Update, &VscpDeviceRouter::handleUpdate);
  registerHandler(server, vscp::Command::Config, &VscpDeviceRouter::handleConfig);
  registerHandler(server, vscp::Command::Control, &VscpDeviceRouter::handleControl);
  registerHandler(server, vscp::Command::Reset, &VscpDeviceRouter::handleReset);
}

void VscpDeviceRouter::registerHandler(vscp::Server& server, vscp::Command command, DeviceHandler handler) {
  server.on(command, [this, command, handler](const vscp::Request& request, vscp::Transport& transport) {
    session_->poll(); // Expire ownership before a queued command can renew a stale lease.
    if (!session_->availableTo(transport)) return vscp::Response::fail("Board busy: another client owns control");
    if (command != vscp::Command::Init && !session_->owns(transport))
      return vscp::Response::fail("Control session required: send INIT");
    auto response = (this->*handler)(request);
    if (command == vscp::Command::Init) {
      if (response.status == vscp::Status::Ok) session_->acquire(transport);
      else if (session_->owns(transport)) stopAllDevices();
    } else session_->activity(transport);
    return response;
  });
}

void VscpDeviceRouter::stopDevice(RegisteredDevice& device) {
  if (device.connected) {
    device.device->detach(); // Do not call RESET: servos/steppers could move.
    if (device.uid[0] == 'A') {
      // Keep active-high driver inputs firmly OFF rather than floating.
      for (const int pin : device.pins) {
        ledcDetachPin(pin);
        digitalWrite(pin, LOW);
        pinMode(pin, OUTPUT);
      }
    }
  }
  device.connected = false;
  device.initialized = false;
  device.pins.clear();
}

void VscpDeviceRouter::stopAllDevices() {
  for (size_t index = 0; index < deviceCount_; ++index) stopDevice(devices_[index]);
}

void VscpDeviceRouter::poll() {
  if (session_) session_->poll(); // Stop stale devices before servicing their motion.
  for (size_t index = 0; index < deviceCount_; ++index) {
    if (devices_[index].connected) devices_[index].device->service();
  }
}

RegisteredDevice* VscpDeviceRouter::findDevice(const String& uid) {
  for (size_t index = 0; index < deviceCount_; ++index) {
    if (uid == devices_[index].uid) return &devices_[index];
  }
  return nullptr;
}

bool VscpDeviceRouter::parsePins(String rawPins, std::vector<int>& pins) {
  pins.clear();
  rawPins.trim();
  rawPins.replace(" ", "");
  if (rawPins.length() == 0) return false;

  int cursor = 0;
  while (cursor < rawPins.length() && pins.size() < 4) {
    int separator = rawPins.indexOf(',', cursor);
    if (separator < 0) separator = rawPins.length();
    const String token = rawPins.substring(cursor, separator);
    if (token.length() == 0) return false;
    for (size_t index = 0; index < token.length(); ++index) {
      if (!isDigit(token.charAt(index))) return false;
    }
    pins.push_back(token.toInt());
    cursor = separator + 1;
  }
  return cursor >= rawPins.length() && !pins.empty();
}

std::vector<DeviceParameter> VscpDeviceRouter::operationParameters(const vscp::Request& request) {
  std::vector<DeviceParameter> parameters;
  for (const auto& parameter : request.parameters) {
    if (parameter.first == "type" || parameter.first == "id" ||
        parameter.first == "api" || parameter.first == "app" ||
        parameter.first == "db" || parameter.first == "pin" ||
        parameter.first == "pins") {
      continue;
    }
    parameters.push_back({parameter.first, parameter.second});
  }
  return parameters;
}

vscp::Response VscpDeviceRouter::handleInit(const vscp::Request& request) {
  const String requestedApi = request.value("api");
  if (requestedApi.length() > 0 && requestedApi != vscp::API_VERSION) {
    return vscp::Response::fail(String("API mismatch: expected ") + vscp::API_VERSION);
  }
  vscp::Response response = vscp::Response::ok();
  response.parameters["api"] = vscp::API_VERSION;
  return response;
}

vscp::Response VscpDeviceRouter::handleConnect(const vscp::Request& request) {
  const String uid = request.value("id");
  RegisteredDevice* registered = findDevice(uid);
  if (!registered) return vscp::Response::fail("Device not found");

  String rawPins = request.value("pins");
  if (rawPins.length() == 0) rawPins = request.value("pin");
  std::vector<int> pins;
  if (!parsePins(rawPins, pins)) return vscp::Response::fail("Invalid pins");
  const size_t requiredPinCount = registered->device->requiredPinCount();
  if (pins.size() != requiredPinCount) {
    return vscp::Response::fail(
        String("Invalid pin count: expected ") + static_cast<unsigned int>(requiredPinCount));
  }
  for (const int pin : pins) {
    if (!GPIO_IS_VALID_GPIO(pin) || (uid.charAt(0) == 'A' && !GPIO_IS_VALID_OUTPUT_GPIO(pin)))
      return vscp::Response::fail("Invalid GPIO");
    if (std::find(reservedPins_.begin(), reservedPins_.end(), pin) != reservedPins_.end())
      return vscp::Response::fail("GPIO reserved for communication");
    for (size_t index = 0; index < deviceCount_; ++index) {
      const auto& other = devices_[index];
      if (&other == registered || !other.connected) continue;
      if ((uid.charAt(0) == 'A' || other.uid[0] == 'A') &&
          std::find(other.pins.begin(), other.pins.end(), pin) != other.pins.end())
        return vscp::Response::fail("GPIO already used by another device");
    }
  }
  for (size_t index = 0; index < deviceCount_; ++index) {
    const auto& other = devices_[index];
    if (&other != registered && other.connected &&
        registered->device->deviceType() == DeviceType::RgbLed &&
        other.device->deviceType() == DeviceType::RgbLed)
      return vscp::Response::fail("RGB driver already used by another device");
  }

  if (registered->connected) stopDevice(*registered);
  if (uid.charAt(0) == 'A') {
    // Clear a previous GPIO latch/PWM before any driver can switch the pin to OUTPUT.
    for (const int pin : pins) {
      ledcDetachPin(pin);
      digitalWrite(pin, LOW);
      pinMode(pin, OUTPUT);
    }
  }
  registered->pins = pins;
  registered->connected = true; // Track new pins so failed init also takes the safe-stop path.
  registered->initialized = false;
  registered->device->attach(pins);
  if (!registered->device->init()) {
    stopDevice(*registered);
    return vscp::Response::fail("Device initialization failed");
  }

  registered->initialized = true;
  return vscp::Response::ok();
}

vscp::Response VscpDeviceRouter::handleDisconnect(const vscp::Request& request) {
  RegisteredDevice* registered = findDevice(request.value("id"));
  if (!registered) return vscp::Response::fail("Device not found");
  stopDevice(*registered);
  return vscp::Response::ok();
}

vscp::Response VscpDeviceRouter::handleUpdate(const vscp::Request& request) {
  RegisteredDevice* registered = findDevice(request.value("id"));
  if (!registered) return vscp::Response::fail("Device not found");
  if (!registered->connected) return vscp::Response::fail("Device not connected");

  std::vector<DeviceValue> values = registered->device->update();
  if (values.empty()) {
    delay(50);
    values = registered->device->update();
  }
  if (values.empty()) {
    delay(150);
    values = registered->device->update();
  }
  if (values.empty()) return vscp::Response::fail("No content");

  vscp::Response response = vscp::Response::ok();
  for (const auto& value : values) response.parameters[value.key] = value.value;
  return response;
}

vscp::Response VscpDeviceRouter::handleConfig(const vscp::Request& request) {
  RegisteredDevice* registered = findDevice(request.value("id"));
  if (!registered) return vscp::Response::fail("Device not found");
  if (!registered->connected) return vscp::Response::fail("Device not connected");

  std::vector<DeviceParameter> parameters = operationParameters(request);
  if (parameters.empty()) return vscp::Response::fail("Missing config parameters");
  registered->device->config(parameters.data(), static_cast<int>(parameters.size()));
  return vscp::Response::ok();
}

vscp::Response VscpDeviceRouter::handleControl(const vscp::Request& request) {
  RegisteredDevice* registered = findDevice(request.value("id"));
  if (!registered) return vscp::Response::fail("Device not found");
  if (!registered->connected) return vscp::Response::fail("Device not connected");

  std::vector<DeviceParameter> parameters = operationParameters(request);
  if (parameters.empty()) return vscp::Response::fail("Missing control parameters");
  registered->device->control(parameters.data(), static_cast<int>(parameters.size()));
  return vscp::Response::ok();
}

vscp::Response VscpDeviceRouter::handleReset(const vscp::Request& request) {
  const String uid = request.value("id");
  if (uid == "S*" || uid == "A*") {
    const char group = uid.charAt(0);
    for (size_t index = 0; index < deviceCount_; ++index) {
      if (devices_[index].uid[0] == group && devices_[index].connected) devices_[index].device->reset();
    }
    return vscp::Response::ok();
  }

  RegisteredDevice* registered = findDevice(uid);
  if (!registered) return vscp::Response::fail("Device not found");
  if (!registered->connected) return vscp::Response::fail("Device not connected");
  registered->device->reset();
  return vscp::Response::ok();
}
