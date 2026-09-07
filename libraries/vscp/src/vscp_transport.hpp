/**
 * @file vscp_transport.hpp
 * @brief Abstract line transport shared by the VSCP client and server.
 */

#pragma once

#include <Arduino.h>

namespace vscp {

enum class ReadStatus : uint8_t {
  NoData,
  Message,
  MessageTooLong
};

class Transport {
public:
  virtual ~Transport() = default;
  virtual ReadStatus readLine(String& message) = 0;
  virtual void writeLine(const String& message) = 0;
};

}  // namespace vscp
