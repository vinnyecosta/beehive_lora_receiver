#pragma once

#include <Arduino.h>

/**
 * @file ota_config.h
 * @brief Compile-time OTA profile, security, and rollback-health defaults.
 */

/** Development profile identifier. */
#define OTA_PROFILE_DEV 0
/** Production profile identifier. */
#define OTA_PROFILE_PRODUCTION 1

/**
 * Active profile selector.
 *
 * Override from build flags, for example:
 * -DOTA_ACTIVE_PROFILE=OTA_PROFILE_DEV
 */
#ifndef OTA_ACTIVE_PROFILE
#define OTA_ACTIVE_PROFILE OTA_PROFILE_PRODUCTION
#endif

#if (OTA_ACTIVE_PROFILE != OTA_PROFILE_DEV) && \
    (OTA_ACTIVE_PROFILE != OTA_PROFILE_PRODUCTION)
#error "OTA_ACTIVE_PROFILE must be OTA_PROFILE_DEV or OTA_PROFILE_PRODUCTION"
#endif

namespace ota_cfg {

#if OTA_ACTIVE_PROFILE == OTA_PROFILE_PRODUCTION
/** Human-readable profile name for logs. */
static constexpr const char* kProfileName = "production";
/** BLE device name advertised for OTA in production profile. */
static constexpr const char* kDeviceName = "HELTEC-RX-OTA";
/** BLE secure pairing mode enabled by default in production profile. */
static constexpr bool kSecureBle = true;
/** BLE passkey exposed for pairing and OTA authentication. */
static constexpr uint32_t kBlePasskey = 739251;
/** OTA MTU target for production profile. */
static constexpr uint16_t kMtu = 517;
/** Device Information Service enabled in production profile. */
static constexpr bool kEnableDeviceInfoService = true;
/** Boot health-check timeout before rollback on pending image. */
static constexpr uint32_t kBootHealthcheckTimeoutMs = 20000;
#else
/** Human-readable profile name for logs. */
static constexpr const char* kProfileName = "development";
/** BLE device name advertised for OTA in development profile. */
static constexpr const char* kDeviceName = "HELTEC-RX-OTA-DEV";
/** BLE secure pairing mode disabled by default in development profile. */
static constexpr bool kSecureBle = false;
/** BLE passkey exposed for pairing and OTA authentication. */
static constexpr uint32_t kBlePasskey = 123456;
/** OTA MTU target for development profile. */
static constexpr uint16_t kMtu = 247;
/** Device Information Service enabled in development profile. */
static constexpr bool kEnableDeviceInfoService = true;
/** Boot health-check timeout before rollback on pending image. */
static constexpr uint32_t kBootHealthcheckTimeoutMs = 30000;
#endif

/** Device Information Service manufacturer value. */
static constexpr const char* kManufacturer = "Heltec";
/** Device Information Service model value. */
static constexpr const char* kModel = "WiFi LoRa 32 V3";
/** Device Information Service firmware version value. */
static constexpr const char* kFirmwareVersion = "1.1.0";
/**
 * Minimum heap threshold used by startup health checks.
 *
 * The check is intentionally lightweight and should be complemented with
 * application-specific checks (radio init, sensor init, connectivity checks).
 */
static constexpr size_t kMinHealthyFreeHeapBytes = 45000;

}  // namespace ota_cfg
