/**
 * @file constants.h
 * @brief Protocol and bit-level constants shared by modules.
 */

#ifndef BEEHIVE_CONSTANTS_H
#define BEEHIVE_CONSTANTS_H

#include <stdint.h>

/**
 * @brief Telemetry status bitmask flags.
 */
enum StatusFlags : uint8_t {
  STATUS_SENSOR_OK = 1 << 0,
  STATUS_LOW_BATTERY_WARNING = 1 << 1,
  STATUS_EMERGENCY_MODE = 1 << 2,
  STATUS_HAS_PREVIOUS_SAMPLE = 1 << 3,
  STATUS_CONFIG_WINDOW_OPENED = 1 << 4,
  STATUS_CONFIG_APPLIED = 1 << 5,
  STATUS_DEBUG_ENABLED = 1 << 6,
};

/**
 * @brief Telemetry error bitmask flags.
 */
enum ErrorFlags : uint16_t {
  ERROR_NONE = 0,
  ERROR_SENSOR_INIT = 1 << 0,
  ERROR_SENSOR_READ = 1 << 1,
  ERROR_RADIO_INIT = 1 << 2,
  ERROR_TX = 1 << 3,
  ERROR_CONFIG_PARSE = 1 << 4,
  ERROR_CONFIG_APPLY = 1 << 5,
  ERROR_BATTERY = 1 << 6,
  ERROR_STORAGE = 1 << 7,
};

/**
 * @brief Remote configuration command IDs.
 */
enum class ConfigCommandId : uint8_t {
  kSetSleepNormalSec = 0x01,
  kSetFrequencyHz = 0x02,
  kSetSpreadingFactor = 0x03,
  kSetBandwidthX10Khz = 0x04,
  kSetCodingRate = 0x05,
  kSetTxPowerDbm = 0x06,
  kSetNodeId = 0x07,
  kSetDebugEnable = 0x08,
  kSetSensorEnable = 0x09,
  kSetLowBatteryThresholdMv = 0x0A,
  kQueryFirmwareVersion = 0x0B,
  kReboot = 0x0C,
  kFactoryReset = 0x0D,
  kSetSleepEmergencySec = 0x0E,
  kSetWarningBatteryThresholdMv = 0x0F,
};

/**
 * @brief Sample-specific flags.
 */
enum SampleFlags : uint8_t {
  SAMPLE_VALID = 1 << 0,
};

#endif  // BEEHIVE_CONSTANTS_H
