/**
 * @file Sensor.hpp
 * @brief Backward-compatible sensor alias for the unified Device API.
 *
 * New code should include Device.hpp and use Device directly. The alias keeps
 * existing concrete device declarations source-compatible during migration.
 */

#pragma once

#include <Device.hpp>

using Sensor = Device;
