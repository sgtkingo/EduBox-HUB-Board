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
#include "ExclusiveControlSession.hpp"
#include <memory>

class VscpDeviceRouter {
public:
  VscpDeviceRouter(RegisteredDevice* devices, size_t deviceCount,
                   uint32_t leaseMs = 10000, uint32_t probeIntervalMs = 3000,
                   uint32_t probeTimeoutMs = 500,
                   ExclusiveControlSession::Clock clock = nullptr);

  void registerHandlers(vscp::Server& server);
  void poll();
  // Future BLE disconnect events must call this from the protocol owner, not a callback.
  void notifyTransportDisconnected(vscp::Transport& transport) {
    if (session_) session_->release(transport);
  }
  void setReservedPins(std::vector<int> pins) { reservedPins_ = std::move(pins); }
  void setTransportAvailabilityCheck(std::function<bool(const vscp::Transport&)> check) { availability_ = std::move(check); }

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
  void stopDevice(RegisteredDevice& device);
  void stopAllDevices();
  using DeviceHandler = vscp::Response (VscpDeviceRouter::*)(const vscp::Request&);
  void registerHandler(vscp::Server& server, vscp::Command command, DeviceHandler handler);

  RegisteredDevice* devices_;
  size_t deviceCount_;
  uint32_t leaseMs_, probeIntervalMs_, probeTimeoutMs_;
  ExclusiveControlSession::Clock clock_;
  std::unique_ptr<ExclusiveControlSession> session_;
  std::vector<int> reservedPins_;
  std::function<bool(const vscp::Transport&)> availability_;
};
