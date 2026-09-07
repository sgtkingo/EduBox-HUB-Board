/**
 * @file vscp_stream_transport.hpp
 * @brief Non-blocking VSCP framing adapter for an Arduino Stream.
 */

#pragma once

#include "vscp_transport.hpp"
#include "vscp_types.hpp"

namespace vscp {

class StreamTransport : public Transport {
public:
  explicit StreamTransport(Stream& stream, size_t maxMessageSize = MAX_MESSAGE_SIZE);

  ReadStatus readLine(String& message) override;
  void writeLine(const String& message) override;

private:
  Stream& stream_;
  String buffer_;
  size_t maxMessageSize_;
  bool overflowed_ = false;
};

}  // namespace vscp
