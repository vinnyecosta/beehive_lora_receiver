#pragma once

#include <Arduino.h>
#include <ota_config.h>

namespace ota {

/**
 * @brief Runtime configuration for the BLE OTA service.
 *
 * This structure controls the BLE advertising identity, optional security,
 * and optional Device Information Service metadata exposed during OTA.
 */
struct Config {
  /** BLE device name shown to scanners and OTA clients. */
  const char* deviceName = ota_cfg::kDeviceName;
  /** Manufacturer string for the Device Information Service. */
  const char* manufacturer = ota_cfg::kManufacturer;
  /** Model string for the Device Information Service. */
  const char* model = ota_cfg::kModel;
  /** Firmware revision string for the Device Information Service. */
  const char* firmwareVersion = ota_cfg::kFirmwareVersion;
  /** Requested BLE ATT MTU for OTA transfer. */
  uint16_t mtu = ota_cfg::kMtu;
  /** Enables BLE pairing + encrypted/authenticated writes when true. */
  bool secure = ota_cfg::kSecureBle;
  /** Passkey used when secure mode is enabled. */
  uint32_t passkey = ota_cfg::kBlePasskey;
  /** Enables BLE Device Information Service exposure when true. */
  bool enableDeviceInfoService = ota_cfg::kEnableDeviceInfoService;
};

/**
 * @brief Initializes BLE OTA services, callbacks, and advertising.
 *
 * Calling this function more than once is safe; subsequent calls return true
 * when the OTA stack is already started.
 *
 * @param config OTA service configuration.
 * @return true if OTA service is ready, false on initialization failure.
 */
bool begin(const Config& config = Config());

/**
 * @brief OTA periodic hook.
 *
 * Currently implemented as a no-op because the underlying BLE OTA stack is
 * event-driven, but kept for API symmetry and future extension.
 */
void loop();

/**
 * @brief Returns true when the running image still requires boot validation.
 *
 * This is true only after an OTA reboot while the partition state is
 * `ESP_OTA_IMG_PENDING_VERIFY`.
 *
 * @return true if startup health validation is pending, false otherwise.
 */
bool isHealthCheckPending();

/**
 * @brief Marks the current firmware image as healthy and cancels rollback.
 *
 * Safe to call every boot; it only performs OTA state transition when health
 * validation is actually pending.
 *
 * @return true if firmware is healthy or no validation was pending.
 */
bool confirmCurrentFirmwareHealthy();

/**
 * @brief Marks current firmware invalid and triggers rollback reboot.
 *
 * If no validation is pending, this function does nothing.
 */
void failCurrentFirmwareAndRollback();

/**
 * @brief Reports whether OTA has already been initialized.
 *
 * @return true if OTA service is active, false otherwise.
 */
bool isStarted();

}  // namespace ota
