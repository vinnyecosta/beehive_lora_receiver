/**
 * @file payload.h
 * @brief Binary telemetry payload serialization and deserialization.
 */

#ifndef BEEHIVE_PAYLOAD_H
#define BEEHIVE_PAYLOAD_H

#include <stddef.h>
#include <stdint.h>

#include "status.h"

/**
 * @brief Packed sample representation used in payload.
 * Force the compiler to use 1-byte alignment (no gaps between variables)
 */
#pragma pack(push, 1)
struct SamplePayloadV1 {
  int16_t temperatureCentiC;
  uint16_t humidityCentiPct;
  uint16_t weightInKg;
  uint32_t pressureCentiHpa;
  uint16_t batteryMilliVolts;
  uint8_t batteryPercent;
  uint8_t sampleFlags;
  uint8_t wakeupCause;
};

/**
 * @brief Payload header for protocol version 1.
 */
struct PayloadHeaderV1 {
  uint8_t payloadVersion;
  uint8_t nodeId;
  uint32_t packetCounter;
  uint8_t firmwareMajor;
  uint8_t firmwareMinor;
  uint8_t firmwarePatch;
  uint8_t statusFlags;
  uint16_t errorFlags;
  int8_t rssiDbm;
  int8_t snrQuarterDb;
};
#pragma pack(pop)

/**
 * @brief Input used by payload serializer.
 */
struct PayloadBuildInput {
  PayloadHeaderV1 header;
  SamplePayloadV1 currentSample;
  bool includePreviousSample;
  uint32_t previousPacketCounter;
  SamplePayloadV1 previousSample;
};

/**
 * @brief Result from payload deserializer.
 */
struct PayloadDecoded {
  PayloadHeaderV1 header;
  SamplePayloadV1 currentSample;
  bool hasPreviousSample;
  uint32_t previousPacketCounter;
  SamplePayloadV1 previousSample;
};

/** Maximum serialized payload length for V1 with optional previous sample and CRC. */
constexpr size_t kMaxPayloadLengthV1 = sizeof(PayloadHeaderV1) + sizeof(SamplePayloadV1) + sizeof(uint32_t) + sizeof(SamplePayloadV1) + sizeof(uint16_t);

/**
 * @brief Computes CRC16-CCITT over data.
 * @param data Input bytes.
 * @param length Number of bytes.
 * @return CRC16-CCITT value.
 */
uint16_t PayloadCrc16Ccitt(const uint8_t* data, size_t length);

/**
 * @brief Serializes payload data using V1 wire format.
 * @param input Structured payload fields.
 * @param[out] output Destination buffer.
 * @param outputCapacity Destination capacity in bytes.
 * @param[out] outputLength Serialized byte length.
 * @return Status::kOk on success.
 */
Status SerializePayloadV1(const PayloadBuildInput& input,
                          uint8_t* output,
                          size_t outputCapacity,
                          size_t* outputLength);

/**
 * @brief Deserializes payload bytes using V1 wire format.
 * @param data Incoming payload bytes.
 * @param length Incoming payload length.
 * @param[out] decoded Decoded structured payload.
 * @return Status::kOk on success.
 */
Status DeserializePayloadV1(const uint8_t* data,
                            size_t length,
                            PayloadDecoded* decoded);

#endif  // BEEHIVE_PAYLOAD_H
