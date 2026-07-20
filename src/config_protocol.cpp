/**
 * @file config_protocol.cpp
 * @brief Remote configuration frame parser implementation.
 */

#include "config_protocol.h"

#include "constants.h"

namespace {

/**
 * @brief Clamps a value to a closed interval.
 * @tparam T Value type.
 * @param value Input value.
 * @param minValue Minimum allowed value.
 * @param maxValue Maximum allowed value.
 * @return Clamped value.
 */
template <typename T>
T Clamp(T value, T minValue, T maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

/**
 * @brief Reads little-endian uint16 from byte array.
 * @param data Source bytes.
 * @param len Source length.
 * @param offset Start offset.
 * @return Decoded uint16 or 0 when out of bounds.
 */
uint16_t ReadU16(const uint8_t* data, size_t len, size_t offset) {
  if (data == nullptr || (offset + 1) >= len) {
    return 0;
  }
  return static_cast<uint16_t>(data[offset]) |
         (static_cast<uint16_t>(data[offset + 1]) << 8);
}

/**
 * @brief Reads little-endian uint32 from byte array.
 * @param data Source bytes.
 * @param len Source length.
 * @param offset Start offset.
 * @return Decoded uint32 or 0 when out of bounds.
 */
uint32_t ReadU32(const uint8_t* data, size_t len, size_t offset) {
  if (data == nullptr || (offset + 3) >= len) {
    return 0;
  }
  return static_cast<uint32_t>(data[offset]) |
         (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) |
         (static_cast<uint32_t>(data[offset + 3]) << 24);
}

// /**
//  * @brief Clamps runtime config values to supported hardware/protocol ranges.
//  * @param[in,out] cfg Runtime config to sanitize.
//  */
// void ClampRuntimeConfig(AppConfig::RuntimeConfig* cfg) {
//   if (cfg == nullptr) {
//     return;
//   }

//   cfg->sleepNormalSec = Clamp<uint32_t>(cfg->sleepNormalSec, 60UL, 7UL * 24UL * 3600UL);
//   cfg->sleepEmergencySec = Clamp<uint32_t>(cfg->sleepEmergencySec, 60UL, 7UL * 24UL * 3600UL);
//   cfg->frequencyHz = Clamp<uint32_t>(cfg->frequencyHz, 150000000UL, 960000000UL);
//   cfg->bandwidthKhzX10 = Clamp<uint16_t>(cfg->bandwidthKhzX10, 78, 5000);
//   cfg->spreadingFactor = Clamp<uint8_t>(cfg->spreadingFactor, 5, 12);
//   cfg->codingRate = Clamp<uint8_t>(cfg->codingRate, 5, 8);
//   cfg->txPowerDbm = Clamp<int8_t>(cfg->txPowerDbm, -9, 22);
//   cfg->lowBatteryMv = Clamp<uint16_t>(cfg->lowBatteryMv, 2800, 4100);
//   cfg->warningBatteryMv = Clamp<uint16_t>(cfg->warningBatteryMv, cfg->lowBatteryMv, 4300);
// }

// }  // namespace

// Status ConfigProtocol::ParseAndApply(const uint8_t* frame,
//                                      size_t frameLength,
//                                      AppConfig::RuntimeConfig* config,
//                                      ConfigProtocolResult* result,
//                                      uint16_t* errorFlags) const {
//   if (frame == nullptr || config == nullptr || result == nullptr || errorFlags == nullptr) {
//     return Status::kInvalidArgument;
//   }

//   result->changed = false;
//   result->rebootRequested = false;
//   result->factoryResetRequested = false;

//   if (frameLength < 2) {
//     *errorFlags |= ERROR_CONFIG_PARSE;
//     return Status::kParseError;
//   }

//   if (frame[0] != AppConfig::kConfigProtocolVersion) {
//     *errorFlags |= ERROR_CONFIG_PARSE;
//     return Status::kUnsupported;
//   }

//   const uint8_t commandCount = frame[1];
//   size_t index = 2;

//   for (uint8_t commandIndex = 0; commandIndex < commandCount; ++commandIndex) {
//     if (index + 2 > frameLength) {
//       *errorFlags |= ERROR_CONFIG_PARSE;
//       return Status::kParseError;
//     }

//     const uint8_t commandId = frame[index++];
//     const uint8_t valueLen = frame[index++];
//     if (index + valueLen > frameLength) {
//       *errorFlags |= ERROR_CONFIG_PARSE;
//       return Status::kParseError;
//     }

//     const Status status = ApplyCommand(commandId,
//                                        &frame[index],
//                                        valueLen,
//                                        config,
//                                        result,
//                                        errorFlags);
//     if (status != Status::kOk) {
//       return status;
//     }

//     index += valueLen;
//   }

//   ClampRuntimeConfig(config);
//   return Status::kOk;
// }

// Status ConfigProtocol::ApplyCommand(uint8_t commandId,
//                                     const uint8_t* value,
//                                     uint8_t valueLen,
//                                     AppConfig::RuntimeConfig* config,
//                                     ConfigProtocolResult* result,
//                                     uint16_t* errorFlags) const {
//   if (value == nullptr || config == nullptr || result == nullptr || errorFlags == nullptr) {
//     return Status::kInvalidArgument;
//   }

//   switch (static_cast<ConfigCommandId>(commandId)) {
//     case ConfigCommandId::kSetSleepNormalSec:
//       if (valueLen != 4) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->sleepNormalSec = ReadU32(value, valueLen, 0);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetFrequencyHz:
//       if (valueLen != 4) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->frequencyHz = ReadU32(value, valueLen, 0);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetSpreadingFactor:
//       if (valueLen != 1) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->spreadingFactor = value[0];
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetBandwidthX10Khz:
//       if (valueLen != 2) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->bandwidthKhzX10 = ReadU16(value, valueLen, 0);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetCodingRate:
//       if (valueLen != 1) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->codingRate = value[0];
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetTxPowerDbm:
//       if (valueLen != 1) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->txPowerDbm = static_cast<int8_t>(value[0]);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetNodeId:
//       if (valueLen != 1) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->nodeId = value[0];
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetDebugEnable:
//       if (valueLen != 1) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->debugEnabled = (value[0] != 0);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetSensorEnable:
//       if (valueLen != 1) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->sensorEnabled = (value[0] != 0);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetLowBatteryThresholdMv:
//       if (valueLen != 2) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->lowBatteryMv = ReadU16(value, valueLen, 0);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kQueryFirmwareVersion:
//       return Status::kOk;

//     case ConfigCommandId::kReboot:
//       result->rebootRequested = true;
//       return Status::kOk;

//     case ConfigCommandId::kFactoryReset:
//       result->factoryResetRequested = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetSleepEmergencySec:
//       if (valueLen != 4) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->sleepEmergencySec = ReadU32(value, valueLen, 0);
//       result->changed = true;
//       return Status::kOk;

//     case ConfigCommandId::kSetWarningBatteryThresholdMv:
//       if (valueLen != 2) {
//         *errorFlags |= ERROR_CONFIG_PARSE;
//         return Status::kParseError;
//       }
//       config->warningBatteryMv = ReadU16(value, valueLen, 0);
//       result->changed = true;
//       return Status::kOk;

//     default:
//       *errorFlags |= ERROR_CONFIG_APPLY;
//       return Status::kUnsupported;
//   }
}
