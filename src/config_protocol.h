/**
 * @file config_protocol.h
 * @brief Remote configuration frame parser and applier.
 */

#ifndef BEEHIVE_CONFIG_PROTOCOL_H
#define BEEHIVE_CONFIG_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

// #include "config.h"
#include "status.h"

/**
 * @brief Result flags produced when a configuration frame is parsed.
 */
struct ConfigProtocolResult {
  bool changed;
  bool rebootRequested;
  bool factoryResetRequested;
};

// /**
//  * @brief Parser for remote configuration protocol frames.
//  */
// class ConfigProtocol {
//  public:
//   /**
//    * @brief Parses and applies a configuration frame to runtime config.
//    *
//    * Frame format:
//    * - Byte 0: protocol version
//    * - Byte 1: command count
//    * - Repeated command entries:
//    *   - command_id (u8)
//    *   - value_len (u8)
//    *   - value bytes
//    *
//    * @param frame Incoming frame buffer.
//    * @param frameLength Frame length in bytes.
//    * @param[in,out] config Runtime configuration to update.
//    * @param[out] result Parsed command effects.
//    * @param[in,out] errorFlags Error flag bitfield to update.
//    * @return Status::kOk on success.
//    */
//   Status ParseAndApply(const uint8_t* frame,
//                        size_t frameLength,
//                        AppConfig::RuntimeConfig* config,
//                        ConfigProtocolResult* result,
//                        uint16_t* errorFlags) const;

//  private:
//   /**
//    * @brief Applies one command entry to runtime config.
//    * @param commandId Command identifier.
//    * @param value Command value pointer.
//    * @param valueLen Command value length.
//    * @param[in,out] config Runtime configuration to update.
//    * @param[in,out] result Parsed command effects.
//    * @param[in,out] errorFlags Error flag bitfield to update.
//    * @return Status::kOk on success.
//    */
//   Status ApplyCommand(uint8_t commandId,
//                       const uint8_t* value,
//                       uint8_t valueLen,
//                       AppConfig::RuntimeConfig* config,
//                       ConfigProtocolResult* result,
//                       uint16_t* errorFlags) const;
// };

#endif  // BEEHIVE_CONFIG_PROTOCOL_H
