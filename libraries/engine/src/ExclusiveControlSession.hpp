#pragma once

#include <vscp.hpp>
#include <cstdint>
#include <functional>

// Board policy, not wire protocol: a single transport owns all physical devices.
// All calls must run from the same owner context as Server::poll().
class ExclusiveControlSession {
public:
  using Clock = uint32_t (*)();
  ExclusiveControlSession(vscp::Server& server, std::function<void()> stopDevices,
                          uint32_t leaseMs = 10000, uint32_t probeIntervalMs = 3000,
                          uint32_t probeTimeoutMs = 500, Clock clock = defaultClock)
      : server_(server), stopDevices_(std::move(stopDevices)), clock_(clock),
        leaseMs_(leaseMs), probeIntervalMs_(probeIntervalMs), probeTimeoutMs_(probeTimeoutMs) {}

  bool acquire(vscp::Transport& transport) {
    if (owner_ && owner_ != &transport) return false;
    if (!owner_) {
      owner_ = &transport;
      probePending_ = false;
      lastProbeMs_ = clock_();
    }
    activity(transport);
    return true;
  }
  bool owns(const vscp::Transport& transport) const { return owner_ == &transport; }
  bool availableTo(const vscp::Transport& transport) const { return !owner_ || owns(transport); }
  void activity(const vscp::Transport& transport) {
    if (owns(transport)) lastAliveMs_ = clock_();
  }
  void release(vscp::Transport& transport) {
    // Also invalidate stale sessions belonging to a non-owner, without touching hardware.
    server_.closeSession(transport);
    if (!owns(transport)) return;
    stopDevices_(); // Keep ownership locked until outputs have been stopped.
    owner_ = nullptr;
    probePending_ = false;
  }
  void poll() {
    if (!owner_) return;
    const uint32_t now = clock_();
    if (probePending_) {
      const auto state = server_.pingResult(*owner_).state;
      if (state != vscp::PingState::Pending) {
        probePending_ = false;
        if (state == vscp::PingState::Ok) lastAliveMs_ = now;
      }
    }
    if (static_cast<uint32_t>(now - lastAliveMs_) >= leaseMs_) {
      auto& previousOwner = *owner_;
      // Best-effort notification, but never depend on a successful write to stop locally.
      server_.bye(previousOwner);
      release(previousOwner);
      return;
    }
    if (!probePending_ && static_cast<uint32_t>(now - lastAliveMs_) >= probeIntervalMs_ &&
        static_cast<uint32_t>(now - lastProbeMs_) >= probeIntervalMs_) {
      lastProbeMs_ = now;
      probePending_ = server_.ping(*owner_, probeTimeoutMs_);
    }
  }

private:
  static uint32_t defaultClock() { return static_cast<uint32_t>(vscp::detail::monotonicMilliseconds()); }
  vscp::Server& server_;
  std::function<void()> stopDevices_;
  Clock clock_;
  uint32_t leaseMs_, probeIntervalMs_, probeTimeoutMs_;
  vscp::Transport* owner_ = nullptr;
  uint32_t lastAliveMs_ = 0, lastProbeMs_ = 0;
  bool probePending_ = false;
};
