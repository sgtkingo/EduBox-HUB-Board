/**
 * @file vscp_stdio_transport.hpp
 * @brief Blocking VSCP line transport for standard C FILE streams.
 */

#pragma once

#include "../config.hpp"

#if defined(STDIO_H_ENV) && VSCP_ENABLE_STDIO

#include "../vscp_types.hpp"
#include "vscp_transport.hpp"

#include <cstdio>
#include <vector>

namespace vscp {

class StdioLogSink : public LogSink {
public:
  explicit StdioLogSink(FILE* output);
  void writeLogLine(const String& message) override;

private:
  FILE* output_;
};

class StdioTransport : public Transport {
public:
  StdioTransport(FILE* input, FILE* output, size_t maxMessageSize = MAX_MESSAGE_SIZE,
                 LogSink* logSink = nullptr);

protected:
  ReadStatus readLineImpl(String& message) override;
  bool writeLineImpl(const String& message) override;

private:
  void discardLineRemainder();

  FILE* input_;
  FILE* output_;
  size_t maxMessageSize_;
  std::vector<char> buffer_;
};

}  // namespace vscp

#endif
