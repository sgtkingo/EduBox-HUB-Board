/**
 * @file VscpDeviceRouter.cpp
 * @brief Implements VSCP command handlers for the EduBox device registry.
 */

#include <VscpDeviceRouter.hpp>

VscpDeviceRouter::VscpDeviceRouter(RegisteredDevice* devices, size_t deviceCount)
    : devices_(devices), deviceCount_(deviceCount) {}

void VscpDeviceRouter::registerHandlers(vscp::Server& server) {
  server.on(vscp::Command::Init, [this](const vscp::Request& request) { return handleInit(request); });
  server.on(vscp::Command::Connect, [this](const vscp::Request& request) { return handleConnect(request); });
  server.on(vscp::Command::Disconnect, [this](const vscp::Request& request) { return handleDisconnect(request); });
  server.on(vscp::Command::Update, [this](const vscp::Request& request) { return handleUpdate(request); });
  server.on(vscp::Command::Config, [this](const vscp::Request& request) { return handleConfig(request); });
  server.on(vscp::Command::Control, [this](const vscp::Request& request) { return handleControl(request); });
  server.on(vscp::Command::Reset, [this](const vscp::Request& request) { return handleReset(request); });
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

  if (registered->connected) registered->device->detach();
  registered->device->attach(pins);
  if (!registered->device->init()) {
    registered->device->detach();
    registered->connected = false;
    registered->initialized = false;
    registered->pins.clear();
    return vscp::Response::fail("Device initialization failed");
  }

  registered->pins = pins;
  registered->connected = true;
  registered->initialized = true;
  return vscp::Response::ok();
}

vscp::Response VscpDeviceRouter::handleDisconnect(const vscp::Request& request) {
  RegisteredDevice* registered = findDevice(request.value("id"));
  if (!registered) return vscp::Response::fail("Device not found");
  if (!registered->connected) return vscp::Response::fail("Device not connected");

  registered->device->detach();
  registered->connected = false;
  registered->initialized = false;
  registered->pins.clear();
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
      if (devices_[index].uid[0] == group) devices_[index].device->reset();
    }
    return vscp::Response::ok();
  }

  RegisteredDevice* registered = findDevice(uid);
  if (!registered) return vscp::Response::fail("Device not found");
  registered->device->reset();
  return vscp::Response::ok();
}
