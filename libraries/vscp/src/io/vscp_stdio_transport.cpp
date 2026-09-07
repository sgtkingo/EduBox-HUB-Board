/**
 * @file vscp_stdio_transport.cpp
 * @brief Implements blocking VSCP framing over standard C FILE streams.
 */

#include "vscp_stdio_transport.hpp"

#if defined(STDIO_H_ENV) && VSCP_ENABLE_STDIO

#include <climits>
#include <stdexcept>

namespace vscp {

StdioLogSink::StdioLogSink(FILE* output) : output_(output) {
  if (!output_) throw std::invalid_argument("VSCP log stream must not be null");
}

void StdioLogSink::writeLogLine(const String& message) {
  std::fputs(detail::stringData(message), output_);
  std::fputc('\n', output_);
  std::fflush(output_);
}

StdioTransport::StdioTransport(FILE* input, FILE* output, size_t maxMessageSize,
                               LogSink* logSink)
    : Transport(logSink), input_(input), output_(output), maxMessageSize_(maxMessageSize),
      buffer_(maxMessageSize + 2, '\0') {
  if (!input_ || !output_) throw std::invalid_argument("VSCP stdio streams must not be null");
  if (buffer_.size() > static_cast<size_t>(INT_MAX)) {
    throw std::length_error("VSCP stdio buffer exceeds the supported size");
  }
}

void StdioTransport::discardLineRemainder() {
  int character = 0;
  while ((character = std::fgetc(input_)) != '\n' && character != EOF) {}
}

ReadStatus StdioTransport::readLineImpl(String& message) {
  message.clear();
  if (!std::fgets(buffer_.data(), static_cast<int>(buffer_.size()), input_)) {
    return ReadStatus::NoData;
  }

  message = buffer_.data();
  const bool completeLine = !message.empty() && message[message.length() - 1] == '\n';
  if (!completeLine && !std::feof(input_)) {
    discardLineRemainder();
    return ReadStatus::MessageTooLong;
  }

  detail::trimString(message);
  if (message.length() > maxMessageSize_) return ReadStatus::MessageTooLong;
  return message.empty() ? ReadStatus::NoData : ReadStatus::Message;
}

bool StdioTransport::writeLineImpl(const String& message) {
  const int messageResult = std::fputs(detail::stringData(message), output_);
  const int newlineResult = std::fputc('\n', output_);
  const int flushResult = std::fflush(output_);
  return messageResult >= 0 && newlineResult != EOF && flushResult == 0;
}

}  // namespace vscp

#endif
