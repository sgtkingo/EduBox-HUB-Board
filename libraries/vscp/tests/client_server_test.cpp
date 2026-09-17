#include "vscp.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

namespace {

class MemoryTransport final : public vscp::Transport {
public:
  void connectTo(MemoryTransport& peer) { peer_ = &peer; }

protected:
  vscp::ReadStatus readLineImpl(vscp::String& message) override {
    std::lock_guard<std::mutex> lock(mutex_);
    if (messages_.empty()) return vscp::ReadStatus::NoData;
    message = messages_.front();
    messages_.pop_front();
    return vscp::ReadStatus::Message;
  }

  bool writeLineImpl(const vscp::String& message) override {
    if (!peer_) return false;
    std::lock_guard<std::mutex> lock(peer_->mutex_);
    peer_->messages_.push_back(message);
    return true;
  }

private:
  MemoryTransport* peer_ = nullptr;
  std::mutex mutex_;
  std::deque<vscp::String> messages_;
};

void testIostreamMessageLimit() {
  std::istringstream terminatedInput("123456789\n?x=1\n");
  std::ostringstream terminatedOutput;
  vscp::IostreamTransport terminatedTransport(terminatedInput, terminatedOutput, 8);
  vscp::String message;
  assert(terminatedTransport.readLine(message) == vscp::ReadStatus::MessageTooLong);
  assert(terminatedTransport.readLine(message) == vscp::ReadStatus::Message);
  assert(message == "?x=1");

  std::istringstream unterminatedInput("123456789");
  std::ostringstream unterminatedOutput;
  vscp::IostreamTransport unterminatedTransport(unterminatedInput, unterminatedOutput, 8);
  assert(unterminatedTransport.readLine(message) == vscp::ReadStatus::MessageTooLong);
  assert(unterminatedTransport.readLine(message) == vscp::ReadStatus::NoData);
}

void testDesktopClientTimeouts() {
  std::istringstream iostreamInput;
  std::ostringstream iostreamOutput;
  vscp::IostreamTransport iostreamTransport(iostreamInput, iostreamOutput);
  vscp::Client iostreamClient(iostreamTransport, 10);
  const auto iostreamStartedAt = std::chrono::steady_clock::now();
  const vscp::ResponseStatus iostreamResponse = iostreamClient.init();
  const auto iostreamElapsed = std::chrono::steady_clock::now() - iostreamStartedAt;
  assert(iostreamResponse.error == "Response timeout");
  assert(iostreamElapsed < std::chrono::milliseconds(250));

  const char* stdioInputPath = "vscp_stdio_input_test.tmp";
  const char* stdioOutputPath = "vscp_stdio_output_test.tmp";
  FILE* stdioSeed = std::fopen(stdioInputPath, "wb");
  assert(stdioSeed != nullptr);
  assert(std::fputs("123456789\n?x=1\n", stdioSeed) >= 0);
  std::fclose(stdioSeed);

  FILE* stdioInput = std::fopen(stdioInputPath, "rb");
  FILE* stdioOutput = std::fopen(stdioOutputPath, "w+b");
  assert(stdioInput != nullptr);
  assert(stdioOutput != nullptr);
  {
    vscp::StdioTransport stdioTransport(stdioInput, stdioOutput, 8);
    vscp::String message;
    assert(stdioTransport.readLine(message) == vscp::ReadStatus::MessageTooLong);
    assert(stdioTransport.readLine(message) == vscp::ReadStatus::Message);
    assert(message == "?x=1");

    vscp::Client stdioClient(stdioTransport, 10);
    const auto stdioStartedAt = std::chrono::steady_clock::now();
    const vscp::ResponseStatus stdioResponse = stdioClient.init();
    const auto stdioElapsed = std::chrono::steady_clock::now() - stdioStartedAt;
    assert(stdioResponse.error == "Response timeout");
    assert(stdioElapsed < std::chrono::milliseconds(250));
  }
  std::fclose(stdioInput);
  std::fclose(stdioOutput);
  std::remove(stdioInputPath);
  std::remove(stdioOutputPath);
}

}  // namespace

int main() {
  testIostreamMessageLimit();
  testDesktopClientTimeouts();

  const vscp::String initRequest = vscp::Codec::buildRequest(
      vscp::Command::Init,
      vscp::Parameters{{"api", vscp::API_VERSION}, {"app", "signal-twin"}});
  assert(initRequest.find("?type=INIT") == 0);

  MemoryTransport stripSender;
  MemoryTransport stripReceiver;
  stripSender.connectTo(stripReceiver);
  stripReceiver.connectTo(stripSender);
  vscp::String dirtyMessage;
  dirtyMessage += static_cast<char>(1);
  dirtyMessage += "  ?type=INIT&api=1.5  ";
  dirtyMessage += static_cast<char>(127);
  stripSender.writeLine(dirtyMessage);
  vscp::String cleanMessage;
  assert(stripReceiver.readLine(cleanMessage) == vscp::ReadStatus::Message);
  assert(cleanMessage == "?type=INIT&api=1.5");

  MemoryTransport clientTransport;
  MemoryTransport serverTransport;
  clientTransport.connectTo(serverTransport);
  serverTransport.connectTo(clientTransport);

  vscp::Server server;
  server.addTransport(serverTransport);
  std::map<vscp::String, vscp::String> configuredValues;
  std::map<vscp::String, vscp::String> controlledValues;
  bool connected = false;

  server.on(vscp::Command::Init, [](const vscp::Request& request) {
    return request.value("api") == vscp::API_VERSION
               ? vscp::Response::ok()
               : vscp::Response::fail("API mismatch");
  });
  server.on(vscp::Command::Connect, [&](const vscp::Request& request) {
    if (request.value("pins").empty()) return vscp::Response::fail("Missing pins");
    connected = true;
    return vscp::Response::ok();
  });
  server.on(vscp::Command::Update, [](const vscp::Request& request) {
    if (request.value("id") == "missing") return vscp::Response::fail("Device not found");
    if (request.value("id") == "mismatch") {
      vscp::Response response = vscp::Response::ok();
      response.parameters["id"] = "other";
      return response;
    }
    vscp::Response response = vscp::Response::ok();
    response.parameters["temperature"] = "23.5";
    return response;
  });
  server.on(vscp::Command::Config, [&](const vscp::Request& request) {
    configuredValues["speed"] = request.value("speed");
    return vscp::Response::ok();
  });
  server.on(vscp::Command::Control, [&](const vscp::Request& request) {
    controlledValues["set_point"] = request.value("set_point");
    return vscp::Response::ok();
  });
  server.on(vscp::Command::Reset, [&](const vscp::Request&) {
    configuredValues.clear();
    controlledValues.clear();
    return vscp::Response::ok();
  });
  server.on(vscp::Command::Disconnect, [&](const vscp::Request&) {
    connected = false;
    return vscp::Response::ok();
  });

  std::atomic<bool> running{true};
  std::thread serverThread([&]() {
    while (running.load()) {
      server.poll();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  vscp::Client client(clientTransport, 100);
  const vscp::ResponseStatus beforeInit = client.update("S01");
  assert(beforeInit.status == vscp::Status::Error);
  assert(beforeInit.error == "Protocol not initialized");

  const vscp::ResponseStatus init = client.init("signal-twin", "1.4");
  assert(init.status == vscp::Status::Ok);
  assert(client.isInitialized());

  const vscp::ResponseStatus connect = client.connect("S01", "1,2");
  assert(connect.status == vscp::Status::Ok);
  assert(connected);

  const vscp::ResponseStatus update = client.update("S01");
  assert(update.status == vscp::Status::Ok);
  assert(update.parameters.at("id") == "S01");
  assert(update.parameters.at("temperature") == "23.5");

  const vscp::ResponseStatus config = client.config(
      "S01", vscp::Parameters{{"speed", "5"}});
  assert(config.status == vscp::Status::Ok);
  assert(configuredValues.at("speed") == "5");

  const vscp::ResponseStatus control = client.control(
      "S01", vscp::Parameters{{"set_point", "35"}});
  assert(control.status == vscp::Status::Ok);
  assert(controlledValues.at("set_point") == "35");

  const vscp::ResponseStatus missing = client.update("missing");
  assert(missing.status == vscp::Status::Error);
  assert(missing.parameters.at("id") == "missing");

  const vscp::ResponseStatus mismatch = client.update("mismatch");
  assert(mismatch.status == vscp::Status::Error);
  assert(mismatch.error == "Response UID mismatch");

  const vscp::ResponseStatus reset = client.reset("S01");
  assert(reset.status == vscp::Status::Ok);
  assert(configuredValues.empty());
  assert(controlledValues.empty());

  const vscp::ResponseStatus disconnect = client.disconnect("S01");
  assert(disconnect.status == vscp::Status::Ok);
  assert(!connected);

  running.store(false);
  serverThread.join();
}
