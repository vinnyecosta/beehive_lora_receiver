/**
 * @file status.h
 * @brief Common status codes used by all firmware modules.
 */

#ifndef BEEHIVE_STATUS_H
#define BEEHIVE_STATUS_H

#include <stdint.h>

/**
 * @brief Module return status codes.
 */
enum class Status : uint8_t {
  kOk = 0,
  kInvalidArgument,
  kSensorError,
  kRadioError,
  kAdcError,
  kTimeout,
  kParseError,
  kStorageError,
  kCrcError,
  kUnsupported,
  kInternalError,
};

#endif  // BEEHIVE_STATUS_H
