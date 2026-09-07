/**
 * @file vscp_stream_transport.cpp
 * @brief Implementation of non-blocking line framing over Arduino Stream.
 */

#include "vscp_stream_transport.hpp"

namespace vscp {

StreamTransport::StreamTransport(Stream& stream, size_t maxMessageSize)
    : stream_(stream), maxMessageSize_(maxMessageSize) {
  buffer_.reserve(maxMessageSize_);
}

ReadStatus StreamTransport::readLine(String& message) {
  message = "";
  while (stream_.available()) {
    const int byteValue = stream_.read();
    if (byteValue < 0) break;

    if (byteValue == '\n' || byteValue == '\r' || byteValue == 0) {
      if (overflowed_) {
        overflowed_ = false;
        buffer_ = "";
        return ReadStatus::MessageTooLong;
      }
      if (buffer_.length() == 0) continue;

      message = buffer_;
      buffer_ = "";
      message.trim();
      if (message.length() > 0) return ReadStatus::Message;
      continue;
    }

    if (overflowed_) continue;
    if (buffer_.length() >= maxMessageSize_) {
      overflowed_ = true;
      buffer_ = "";
      continue;
    }
    if (byteValue >= 32 && byteValue <= 126) buffer_ += static_cast<char>(byteValue);
  }
  return ReadStatus::NoData;
}

void StreamTransport::writeLine(const String& message) {
  stream_.println(message);
}

}  // namespace vscp
