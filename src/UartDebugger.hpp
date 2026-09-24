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
      log(direction, message);
    }
  }

private:
  Print& output_;
};

/** Tracks one protocol endpoint so INIT and failures have useful context. */
class ProtocolDebugger {
public:
  ProtocolDebugger(UartDebugger& debugger, const char* transportName)
      : debugger_(debugger), transportName_(transportName) {}

  void received(const String& message) {
    debugger_.frame("RX", String(transportName_) + " " + message);

    vscp::Request request;
    String parseError;
    if (!vscp::Codec::parseRequest(message, request, parseError)) return;
    initPending_ = request.command == vscp::Command::Init;
    if (!initPending_) return;

    debugger_.log("DEBUG", String("INIT started transport=") + transportName_ +
        " app=" + valueOrDash(request, "app") +
        " api=" + valueOrDash(request, "api") +
        " db=" + valueOrDash(request, "db"));
  }

  void sent(const String& message, bool written) {
    if (written) debugger_.frame("TX", String(transportName_) + " " + message);

    vscp::ResponseStatus response;
    String parseError;
    const bool parsed = vscp::Codec::parseResponse(message, response, parseError);
    if (!written && initPending_) {
      debugger_.log("ERROR", String("INIT response write failed transport=") + transportName_);
    } else if (!written) {
      debugger_.log("ERROR", String("VSCP transport=") + transportName_ +
          " response write failed");
    } else if (parsed && initPending_) {
      if (response.status == vscp::Status::Ok) {
        debugger_.log("DEBUG", String("INIT completed transport=") + transportName_ +
            " api=" + responseValueOrDash(response, "api"));
      } else {
        debugger_.log("ERROR", String("INIT failed transport=") + transportName_ +
            " error=" + errorOrStatus(response));
      }
    } else if (parsed && response.status == vscp::Status::Error) {
      const auto idEntry = response.parameters.find("id");
      const String id = idEntry != response.parameters.end() ? idEntry->second : String("-");
      debugger_.log("ERROR", String("VSCP transport=") + transportName_ +
          " id=" + id + " error=" + errorOrStatus(response));
    }
    if (parsed && response.status == vscp::Status::Ok) warnInvalidValues(response);
    initPending_ = false;
  }

  void readError(const char* error) {
    debugger_.log("ERROR", String("VSCP transport=") + transportName_ + " error=" + error);
  }

private:
  static String valueOrDash(const vscp::Request& request, const char* key) {
    const String value = request.value(key);
    return value.length() ? value : String("-");
  }

  static String responseValueOrDash(const vscp::ResponseStatus& response, const char* key) {
    const auto entry = response.parameters.find(key);
    return entry != response.parameters.end() ? entry->second : String("-");
  }

  static String errorOrStatus(const vscp::ResponseStatus& response) {
    return response.error.length() ? response.error : String("status=0");
  }

  void warnInvalidValues(const vscp::ResponseStatus& response) {
    const auto idEntry = response.parameters.find("id");
    const String id = idEntry != response.parameters.end() ? idEntry->second : String("-");
    for (const auto& value : response.parameters) {
      String normalized = value.second;
      normalized.toLowerCase();
      if (normalized == "nan" || normalized == "inf" || normalized == "-inf") {
        debugger_.log("WARN", String("VSCP transport=") + transportName_ +
            " id=" + id + " invalid sensor value " + value.first + "=" + value.second);
      }
    }
  }

  UartDebugger& debugger_;
  const char* transportName_;
  bool initPending_ = false;
};

class DebugStreamTransport : public vscp::StreamTransport {
public:
  DebugStreamTransport(Stream& stream, UartDebugger& debugger, const char* transportName)
      : vscp::StreamTransport(stream), protocolDebugger_(debugger, transportName) {}

protected:
  vscp::ReadStatus readLineImpl(String& message) override {
    const auto status = vscp::StreamTransport::readLineImpl(message);
    if (status == vscp::ReadStatus::Message) protocolDebugger_.received(message);
    else if (status == vscp::ReadStatus::MessageTooLong)
      protocolDebugger_.readError("request too long");
    return status;
  }

  bool writeLineImpl(const String& message) override {
    const bool written = vscp::StreamTransport::writeLineImpl(message);
    protocolDebugger_.sent(message, written);
    return written;
  }

private:
  ProtocolDebugger protocolDebugger_;
};
