/**
 * @file libs.hpp
 * @brief Central include list for EduBox sensor and actuator implementations.
 */

#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BMP280.h>

#include "Adafruit_TCS34725.h"
#include "ESP32Servo.h"
#include "Sensor.hpp"

// knihovny aktuátorů
#include "Stepper.hpp"
#include "SG90.hpp"
#include "DC.hpp"
#include "TwoColor.hpp"
#include "TwoColorMini.hpp"
#include "RGB.hpp"
#include "7color.hpp"
#include "Laser.hpp"
#include "BuzzP.hpp"
#include "BuzzA.hpp"
#include "IRtx.hpp"

// knihovny senzorů
#include "Senzor_Encoder.hpp"
#include "Senzor_DS18B20.hpp"
#include "Senzor_DHT11.hpp"
#include "Senzor_Ahall.hpp"
#include "Senzor_HC_SR04.hpp"
#include "Senzor_Antc.hpp"
#include "Senzor_PHresistance.hpp"
#include "Senzor_Joystick.hpp"
#include "Senzor_HallLin.hpp"
#include "Senzor_TCS34725.hpp"
#include "Senzor_GP2Y0A21YK0F.hpp"
#include "Senzor_MicSmall.hpp"
#include "Senzor_MicBig.hpp"
#include "Senzor_Heartbeat.hpp"
#include "Senzor_BMP180.hpp"
#include "Senzor_BMP.hpp"
#include "Senzor_DigitalRead.hpp"
#include "Senzor_IRrx.hpp"
