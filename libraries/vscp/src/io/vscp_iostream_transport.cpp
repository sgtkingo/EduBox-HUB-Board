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
    : Transport(logSink), input_(input), output_(output), maxMessageSize_(maxMessageSize) {}

ReadStatus IostreamTransport::readLineImpl(String& message) {
  message.clear();
  if (!std::getline(input_, message)) return ReadStatus::NoData;

  detail::trimString(message);
  if (message.length() > maxMessageSize_) return ReadStatus::MessageTooLong;
  return message.empty() ? ReadStatus::NoData : ReadStatus::Message;
}

bool IostreamTransport::writeLineImpl(const String& message) {
  output_ << message << '\n';
  output_.flush();
  return output_.good();
}

}  // namespace vscp

#endif
