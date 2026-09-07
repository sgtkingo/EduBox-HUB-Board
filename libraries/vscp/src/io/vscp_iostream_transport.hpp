/**
 * @file vscp_iostream_transport.hpp
 * @brief Blocking VSCP line transport for standard C++ input and output streams.
 */

#pragma once

#include "../config.hpp"

#if defined(STDIO_H_ENV) && VSCP_ENABLE_IOSTREAM

#include "../vscp_types.hpp"
#include "vscp_transport.hpp"

#include <iostream>

namespace vscp {

class IostreamLogSink : public LogSink {
public:
  explicit IostreamLogSink(std::ostream& output) : output_(output) {}
  void writeLogLine(const String& message) override;

private:
  std::ostream& output_;
};

class IostreamTransport : public Transport {
public:
  explicit IostreamTransport(std::iostream& stream,
                             size_t maxMessageSize = MAX_MESSAGE_SIZE,
                             LogSink* logSink = nullptr);
  IostreamTransport(std::istream& input, std::ostream& output,
                    size_t maxMessageSize = MAX_MESSAGE_SIZE,
                    LogSink* logSink = nullptr);

protected:
  ReadStatus readLineImpl(String& message) override;
  bool writeLineImpl(const String& message) override;

private:
  std::istream& input_;
  std::ostream& output_;
  size_t maxMessageSize_;
};

}  // namespace vscp

#endif
