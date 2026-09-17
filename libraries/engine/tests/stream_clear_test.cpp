#include <io/vscp_stream_transport.hpp>
#include <cassert>
#include <deque>
#include <iostream>
class TestStream : public Stream {
  std::deque<char> rx_;
public:
  void feed(const String& value) { for (char byte : value) rx_.push_back(byte); }
  int available() override { return static_cast<int>(rx_.size()); }
  int read() override { if (rx_.empty()) return -1; char byte = rx_.front(); rx_.pop_front(); return byte; }
};
// Driver glue symbols linked by the shared router test harness, unused here.
void IRtx_control(int, uint32_t) {}
void IRtx_reset(int) {}
void Color7_control(int, bool) {}
void Color7_reset(int) {}
void Laser_setPin(int) {}
void Laser_control(bool) {}
void Laser_reset() {}
int main() {
  TestStream stream; vscp::StreamTransport transport(stream, 64);
  String line;
  stream.feed("?type=UPDATE&id=A");
  assert(transport.readLine(line) == vscp::ReadStatus::NoData);
  stream.feed("02\n");
  transport.clearInput();
  assert(stream.available() == 0 && transport.readLine(line) == vscp::ReadStatus::NoData);
  stream.feed("?status=1\n");
  assert(transport.readLine(line) == vscp::ReadStatus::Message && line == "?status=1");
  stream.feed(String(100, 'x'));
  assert(transport.readLine(line) == vscp::ReadStatus::NoData); // Overflow latched.
  transport.clearInput();
  stream.feed("?status=1\n");
  assert(transport.readLine(line) == vscp::ReadStatus::Message && line == "?status=1");
  std::cout << "PASS UART partial/overflow state cleared on transport selection\n";
}
