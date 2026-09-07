/**
 * @file Actuator.hpp
 * @brief Backward-compatible actuator alias for the unified Device API.
 *
 * Sensors and actuators now share Device, including UPDATE, CONFIG, and
 * CONTROL. This alias avoids a flag-day rename in downstream sketches.
 */

#pragma once

#include <Device.hpp>

using Actuator = Device;
