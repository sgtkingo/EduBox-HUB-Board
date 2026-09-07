/**
 * @file vscp_log_sink.hpp
 * @brief Output interface for VSCP transport diagnostics.
 */

#pragma once

#include "../vscp_platform.hpp"

namespace vscp {

class LogSink {
public:
  virtual ~LogSink() = default;
  virtual void writeLogLine(const String& message) = 0;
};

}  // namespace vscp
