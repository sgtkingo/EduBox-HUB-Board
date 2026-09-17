#pragma once
#include <Arduino.h>
class Print {
public:
  virtual ~Print() = default;
  virtual size_t println(const String&) { return 1; }
  virtual size_t print(char) { return 1; }
  virtual size_t print(const String& value) { return value.size(); }
};
class Stream : public Print {
public:
  virtual int available() { return 0; }
  virtual int read() { return -1; }
  virtual void flush() {}
};
