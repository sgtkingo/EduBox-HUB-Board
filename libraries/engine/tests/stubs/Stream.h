#pragma once
#include <Arduino.h>
class Print { public: size_t println(const String&) { return 0; } };
class Stream : public Print {};
