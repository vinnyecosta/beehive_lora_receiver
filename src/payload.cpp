/**
 * @file payload.cpp
 * @brief Payload serializer and deserializer implementation.
 */

#include "payload.h"

namespace {

/**
 * @brief Appends one byte to buffer.
 * @param value Byte value to append.
 * @param[out] output Destination byte buffer.
 * @param capacity Destination buffer capacity.
 * @param[in,out] index Current write cursor.
 * @return Status::kOk when append succeeded.
 */
Status AppendU8(uint8_t value, uint8_t* output, size_t capacity, size_t* index) {
  if (output == nullptr || index == nullptr || *index >= capacity) {
    return Status::kInvalidArgument;
  }
  output[(*index)++] = value;
  return Status::kOk;
}

/**
 * @brief Appends little-endian 16-bit value.
 * @param value Value to append.
 * @param[out] output Destination byte buffer.
 * @param capacity Destination buffer capacity.
 * @param[in,out] index Current write cursor.
 * @return Status::kOk when append succeeded.
 */
Status AppendU16(uint16_t value, uint8_t* output, size_t capacity, size_t* index) {
  Status status = AppendU8(static_cast<uint8_t>(value & 0xFF), output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }
  return AppendU8(static_cast<uint8_t>((value >> 8) & 0xFF), output, capacity, index);
}

/**
 * @brief Appends little-endian 32-bit value.
 * @param value Value to append.
 * @param[out] output Destination byte buffer.
 * @param capacity Destination buffer capacity.
 * @param[in,out] index Current write cursor.
 * @return Status::kOk when append succeeded.
 */
Status AppendU32(uint32_t value, uint8_t* output, size_t capacity, size_t* index) {
  Status status = AppendU8(static_cast<uint8_t>(value & 0xFF), output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }
  status = AppendU8(static_cast<uint8_t>((value >> 8) & 0xFF), output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }
  status = AppendU8(static_cast<uint8_t>((value >> 16) & 0xFF), output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }
  return AppendU8(static_cast<uint8_t>((value >> 24) & 0xFF), output, capacity, index);
}

/**
 * @brief Reads little-endian 16-bit value.
 * @param data Source buffer.
 * @param offset Byte offset.
 * @return Decoded 16-bit value.
 */
uint16_t ReadU16(const uint8_t* data, size_t offset) {
  return static_cast<uint16_t>(data[offset]) |
         (static_cast<uint16_t>(data[offset + 1]) << 8);
}

/**
 * @brief Reads little-endian 32-bit value.
 * @param data Source buffer.
 * @param offset Byte offset.
 * @return Decoded 32-bit value.
 */
uint32_t ReadU32(const uint8_t* data, size_t offset) {
  return static_cast<uint32_t>(data[offset]) |
         (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) |
         (static_cast<uint32_t>(data[offset + 3]) << 24);
}

/**
 * @brief Appends compact sample structure.
 * @param sample Sample structure to serialize.
 * @param[out] output Destination byte buffer.
 * @param capacity Destination buffer capacity.
 * @param[in,out] index Current write cursor.
 * @return Status::kOk when append succeeded.
 */
Status AppendSample(const SamplePayloadV1& sample, uint8_t* output, size_t capacity, size_t* index) {
  Status status = AppendU16(static_cast<uint16_t>(sample.temperatureCentiC), output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU16(sample.humidityCentiPct, output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU16(sample.weightInKg, output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU32(sample.pressureCentiHpa, output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU16(sample.batteryMilliVolts, output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(sample.batteryPercent, output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(sample.sampleFlags, output, capacity, index);
  if (status != Status::kOk) {
    return status;
  }

  return AppendU8(sample.wakeupCause, output, capacity, index);
}

/**
 * @brief Reads compact sample structure.
 * @param data Source byte buffer.
 * @param offset Sample block offset.
 * @param[out] sample Destination parsed sample.
 */
void ReadSample(const uint8_t* data, size_t offset, SamplePayloadV1* sample) {
  sample->temperatureCentiC = static_cast<int16_t>(ReadU16(data, offset));
  sample->humidityCentiPct = ReadU16(data, offset + 2);
  sample->weightInKg = ReadU16(data, offset + 4);
  sample->pressureCentiHpa = ReadU32(data, offset + 6);
  sample->batteryMilliVolts = ReadU16(data, offset + 10);
  sample->batteryPercent = data[offset + 12];
  sample->sampleFlags = data[offset + 13];
  sample->wakeupCause = data[offset + 14];
}

}  // namespace

/**
 * @brief Computes CRC16-CCITT over payload bytes.
 * @param data Input byte buffer.
 * @param length Number of bytes to include.
 * @return CRC16-CCITT checksum.
 */
uint16_t PayloadCrc16Ccitt(const uint8_t* data, size_t length) {
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      if ((crc & 0x8000) != 0) {
        crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
      } else {
        crc <<= 1;
      }
    }
  }

  return crc;
}

/**
 * @brief Serializes telemetry fields into payload V1 wire format.
 * @param input Source payload fields.
 * @param[out] output Destination byte buffer.
 * @param outputCapacity Destination buffer capacity.
 * @param[out] outputLength Serialized length in bytes.
 * @return Status::kOk on successful serialization.
 */
Status SerializePayloadV1(const PayloadBuildInput& input,
                          uint8_t* output,
                          size_t outputCapacity,
                          size_t* outputLength) {
  if (output == nullptr || outputLength == nullptr) {
    return Status::kInvalidArgument;
  }

  size_t index = 0;
  Status status = AppendU8(input.header.payloadVersion, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(input.header.nodeId, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU32(input.header.packetCounter, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(input.header.firmwareMajor, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(input.header.firmwareMinor, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(input.header.firmwarePatch, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(input.header.statusFlags, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU16(input.header.errorFlags, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(static_cast<uint8_t>(input.header.rssiDbm), output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendU8(static_cast<uint8_t>(input.header.snrQuarterDb), output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  status = AppendSample(input.currentSample, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  if (input.includePreviousSample) {
    status = AppendU32(input.previousPacketCounter, output, outputCapacity, &index);
    if (status != Status::kOk) {
      return status;
    }

    status = AppendSample(input.previousSample, output, outputCapacity, &index);
    if (status != Status::kOk) {
      return status;
    }
  }

  const uint16_t crc = PayloadCrc16Ccitt(output, index);
  status = AppendU16(crc, output, outputCapacity, &index);
  if (status != Status::kOk) {
    return status;
  }

  *outputLength = index;
  return Status::kOk;
}

/**
 * @brief Deserializes payload V1 bytes into structured fields.
 * @param data Incoming payload buffer.
 * @param length Incoming payload length.
 * @param[out] decoded Destination decoded fields.
 * @return Status::kOk when payload is valid and decoded.
 */
Status DeserializePayloadV1(const uint8_t* data,
                            size_t length,
                            PayloadDecoded* decoded) {
  if (data == nullptr || decoded == nullptr) {
    return Status::kInvalidArgument;
  }

  if (length < (sizeof(PayloadHeaderV1) + sizeof(SamplePayloadV1) + sizeof(uint16_t))) {
    return Status::kParseError;
  }

  const uint16_t incomingCrc = ReadU16(data, length - 2);
  const uint16_t computedCrc = PayloadCrc16Ccitt(data, length - 2);
  if (incomingCrc != computedCrc) {
    return Status::kCrcError;
  }

  size_t index = 0;
  decoded->header.payloadVersion = data[index++];
  decoded->header.nodeId = data[index++];
  decoded->header.packetCounter = ReadU32(data, index);
  index += 4;
  decoded->header.firmwareMajor = data[index++];
  decoded->header.firmwareMinor = data[index++];
  decoded->header.firmwarePatch = data[index++];
  decoded->header.statusFlags = data[index++];
  decoded->header.errorFlags = ReadU16(data, index);
  index += 2;
  decoded->header.rssiDbm = static_cast<int8_t>(data[index++]);
  decoded->header.snrQuarterDb = static_cast<int8_t>(data[index++]);

  ReadSample(data, index, &decoded->currentSample);
  index += sizeof(SamplePayloadV1);

  decoded->hasPreviousSample = (length > (sizeof(PayloadHeaderV1) + sizeof(SamplePayloadV1) + sizeof(uint16_t)));
  decoded->previousPacketCounter = 0;

  if (decoded->hasPreviousSample) {
    if (length < (sizeof(PayloadHeaderV1) + sizeof(SamplePayloadV1) + sizeof(uint32_t) + sizeof(SamplePayloadV1) + sizeof(uint16_t))) {
      return Status::kParseError;
    }

    decoded->previousPacketCounter = ReadU32(data, index);
    index += 4;
    ReadSample(data, index, &decoded->previousSample);
  }

  return Status::kOk;
}
