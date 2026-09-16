/**
 * @file UartDebugger.hpp
 * @brief Application diagnostics, kept separate from UART protocol frames.
 */
#pragma once

#include "BoardConfig.hpp"
#include <vscp.hpp>

class UartDebugger : public vscp::LogSink {
public:
  explicit UartDebugger(Print& output) : output_(output) {}

  void log(const char* level, const String& message) {
    if (!uartDebugEnabled) return;
    output_.print("LOG [");
    output_.print(millis());
    output_.print("][");
    output_.print(level);
    output_.print("] ");
    output_.println(message);
  }

  void writeLogLine(const String& message) override { log("ERROR", message); }

  void frame(const char* direction, const String& message) {
    if (uartDebugTraceEnabled) {
      log(direction, String("UART") + vscpUartConfig.port + " " + message);
    }
  }

private:
  Print& output_;
};

class DebugUartTransport : public vscp::StreamTransport {
public:
  DebugUartTransport(Stream& stream, UartDebugger& debugger)
      : vscp::StreamTransport(stream, vscp::MAX_MESSAGE_SIZE,
                              uartDebugEnabled ? &debugger : nullptr),
        debugger_(debugger) {}

protected:
  vscp::ReadStatus readLineImpl(String& message) override {
    const auto status = vscp::StreamTransport::readLineImpl(message);
    if (status == vscp::ReadStatus::Message) debugger_.frame("RX", message);
    return status;
  }

  bool writeLineImpl(const String& message) override {
    const bool written = vscp::StreamTransport::writeLineImpl(message);
    if (!uartDebugEnabled) return written;
    if (written) debugger_.frame("TX", message);

    vscp::ResponseStatus response;
    String parseError;
    if (vscp::Codec::parseResponse(message, response, parseError)) {
      const auto idEntry = response.parameters.find("id");
      const String id = idEntry != response.parameters.end() ? idEntry->second : String("-");
      if (response.status == vscp::Status::Error) {
        debugger_.log("ERROR", String("VSCP id=") + id +
                                  " " + response.error);
      } else {
        for (const auto& value : response.parameters) {
          String normalized = value.second;
          normalized.toLowerCase();
          if (normalized == "nan" || normalized == "inf" || normalized == "-inf") {
            debugger_.log("WARN", String("VSCP id=") + id +
                                     " invalid sensor value " + value.first + "=" + value.second);
          }
        }
      }
    }
    return written;
  }

private:
  UartDebugger& debugger_;
};
