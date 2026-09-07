/**
 * @file vscp_stdio_transport.cpp
 * @brief Implements blocking VSCP framing over standard C FILE streams.
 */

#include "vscp_stdio_transport.hpp"

#if defined(STDIO_H_ENV) && VSCP_ENABLE_STDIO

#include <stdexcept>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

namespace {

bool inputReady(FILE* input) {
#ifdef _WIN32
  const int descriptor = _fileno(input);
  if (descriptor < 0) return false;

  const intptr_t rawHandle = _get_osfhandle(descriptor);
  if (rawHandle == -1) return false;
  HANDLE handle = reinterpret_cast<HANDLE>(rawHandle);
  const DWORD type = GetFileType(handle);
  if (type == FILE_TYPE_DISK) return true;
  if (type == FILE_TYPE_PIPE) {
    DWORD available = 0;
    return PeekNamedPipe(handle, nullptr, 0, nullptr, &available, nullptr) && available > 0;
  }
  if (type == FILE_TYPE_CHAR) {
    COMSTAT status{};
    DWORD errors = 0;
    if (ClearCommError(handle, &errors, &status)) return status.cbInQue > 0;
    return WaitForSingleObject(handle, 0) == WAIT_OBJECT_0;
  }
  return false;
#else
  const int descriptor = fileno(input);
  if (descriptor < 0) return false;
  pollfd descriptorState{descriptor, POLLIN, 0};
  return poll(&descriptorState, 1, 0) > 0 &&
         (descriptorState.revents & (POLLIN | POLLHUP)) != 0;
#endif
}

}  // namespace

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
    : Transport(logSink), input_(input), output_(output), maxMessageSize_(maxMessageSize) {
  if (!input_ || !output_) throw std::invalid_argument("VSCP stdio streams must not be null");
  if (std::setvbuf(input_, nullptr, _IONBF, 0) != 0) {
    throw std::runtime_error("Unable to configure non-blocking VSCP input buffering");
  }
  buffer_.reserve(maxMessageSize_);
}

ReadStatus StdioTransport::readLineImpl(String& message) {
  message.clear();
  bool overflowDetected = false;
  while (inputReady(input_)) {
    const int value = std::fgetc(input_);
    if (value == EOF) break;
    const char character = static_cast<char>(value);

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

bool StdioTransport::writeLineImpl(const String& message) {
  const int messageResult = std::fputs(detail::stringData(message), output_);
  const int newlineResult = std::fputc('\n', output_);
  const int flushResult = std::fflush(output_);
  return messageResult >= 0 && newlineResult != EOF && flushResult == 0;
}

}  // namespace vscp

#endif
