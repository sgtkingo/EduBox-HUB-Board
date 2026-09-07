/**
 * @file vscp_iostream_transport.cpp
 * @brief Implements blocking VSCP framing over standard C++ streams.
 */

#include "vscp_iostream_transport.hpp"

#if defined(STDIO_H_ENV) && VSCP_ENABLE_IOSTREAM

#include <string>

namespace vscp {

void IostreamLogSink::writeLogLine(const String& message) {
  output_ << message << '\n';
  output_.flush();
}

IostreamTransport::IostreamTransport(std::iostream& stream, size_t maxMessageSize,
                                     LogSink* logSink)
    : IostreamTransport(stream, stream, maxMessageSize, logSink) {}

IostreamTransport::IostreamTransport(std::istream& input, std::ostream& output,
                                     size_t maxMessageSize, LogSink* logSink)
    : Transport(logSink), input_(input), output_(output), maxMessageSize_(maxMessageSize) {
  buffer_.reserve(maxMessageSize_);
}

ReadStatus IostreamTransport::readLineImpl(String& message) {
  message.clear();
  std::streambuf* inputBuffer = input_.rdbuf();
  if (!inputBuffer) return ReadStatus::NoData;

  bool overflowDetected = false;
  while (inputBuffer->in_avail() > 0) {
    const std::streambuf::int_type value = inputBuffer->sbumpc();
    if (std::streambuf::traits_type::eq_int_type(
            value, std::streambuf::traits_type::eof())) {
      break;
    }

    const char character = std::streambuf::traits_type::to_char_type(value);
    if (character == '\n' || character == '\r' || character == 0) {
      if (overflowed_) {
        overflowed_ = false;
        if (overflowDetected) return ReadStatus::MessageTooLong;
        continue;
      }
      if (buffer_.empty()) continue;

      message.swap(buffer_);
      detail::trimString(message);
      if (!message.empty()) return ReadStatus::Message;
      continue;
    }

    if (overflowed_) continue;
    if (buffer_.length() >= maxMessageSize_) {
      buffer_.clear();
      overflowed_ = true;
      overflowDetected = true;
      continue;
    }
    if (character >= 32 && character <= 126) buffer_ += character;
  }

  return overflowDetected ? ReadStatus::MessageTooLong : ReadStatus::NoData;
}

bool IostreamTransport::writeLineImpl(const String& message) {
  output_ << message << '\n';
  output_.flush();
  return output_.good();
}

}  // namespace vscp

#endif
