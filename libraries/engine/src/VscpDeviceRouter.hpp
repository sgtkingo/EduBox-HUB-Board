/**
 * @file VscpDeviceRouter.hpp
 * @brief Routes VSCP server commands to registered hardware devices.
 *
 * The router owns protocol-to-domain translation only. Framing, parsing,
 * session initialization, and response serialization remain in the reusable
 * VSCP library.
 */

#pragma once

#include <vscp.hpp>

#include <Device.hpp>

class VscpDeviceRouter {
public:
  VscpDeviceRouter(RegisteredDevice* devices, size_t deviceCount);

  void registerHandlers(vscp::Server& server);

private:
  vscp::Response handleInit(const vscp::Request& request);
  vscp::Response handleConnect(const vscp::Request& request);
  vscp::Response handleDisconnect(const vscp::Request& request);
  vscp::Response handleUpdate(const vscp::Request& request);
  vscp::Response handleConfig(const vscp::Request& request);
  vscp::Response handleControl(const vscp::Request& request);
  vscp::Response handleReset(const vscp::Request& request);

  RegisteredDevice* findDevice(const String& uid);
  static bool parsePins(String rawPins, std::vector<int>& pins);
  static std::vector<DeviceParameter> operationParameters(const vscp::Request& request);

  RegisteredDevice* devices_;
  size_t deviceCount_;
};
