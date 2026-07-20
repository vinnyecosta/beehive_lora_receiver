#include "OtaManager.h"

#include <esp_ota_ops.h>
#include <NimBLEDevice.h>
#include <NimBLEDis.h>
#include <NimBLEOta.h>

namespace {

NimBLEOta bleOta;
NimBLEDis bleDis;
bool otaStarted = false;
bool healthCheckPending = false;
uint32_t securityPasskey = ota_cfg::kBlePasskey;
unsigned long healthCheckStartMs = 0;

/**
 * @brief Returns true if the current partition is awaiting OTA validation.
 *
 * @return true when current image state is `ESP_OTA_IMG_PENDING_VERIFY`.
 */
bool isPartitionPendingVerify() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  if (running == nullptr) {
    return false;
  }

  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(running, &state) != ESP_OK) {
    return false;
  }

  return state == ESP_OTA_IMG_PENDING_VERIFY;
}

/**
 * @brief Performs lightweight built-in startup checks used by rollback flow.
 *
 * @return true if built-in checks pass.
 */
bool internalStartupHealthChecks() {
  if (ESP.getFreeHeap() < ota_cfg::kMinHealthyFreeHeapBytes) {
    Serial.printf("Startup health check failed: free heap too low (%u)\n",
                  static_cast<unsigned int>(ESP.getFreeHeap()));
    return false;
  }

  return true;
}

/**
 * @brief Returns true if the configured health-check timeout has elapsed.
 *
 * @return true when a pending health check timed out.
 */
bool isHealthCheckTimedOut() {
  if (!healthCheckPending) {
    return false;
  }

  if (ota_cfg::kBootHealthcheckTimeoutMs == 0) {
    return false;
  }

  return (millis() - healthCheckStartMs) >= ota_cfg::kBootHealthcheckTimeoutMs;
}

/**
 * @brief Maps OTA stop/start/error reasons into short printable strings.
 *
 * @param reason OTA reason code provided by the NimBLEOta callback API.
 * @return Human-readable static string for logging.
 */
const char* reasonToString(NimBLEOta::Reason reason) {
  switch (reason) {
    case NimBLEOta::StartCmd:
      return "start-cmd";
    case NimBLEOta::StopCmd:
      return "stop-cmd";
    case NimBLEOta::Disconnected:
      return "disconnected";
    case NimBLEOta::Reconnected:
      return "reconnected";
    case NimBLEOta::FlashError:
      return "flash-error";
    case NimBLEOta::LengthError:
      return "length-error";
    default:
      return "unknown";
  }
}

class BleServerCallbacks : public NimBLEServerCallbacks {
 public:
  /**
   * @brief Handles BLE central connection events.
   *
   * @param pServer Active NimBLE server instance.
   * @param connInfo Connection metadata from the central device.
   */
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    (void)pServer;
    Serial.printf("BLE client connected: %s\n", connInfo.getAddress().toString().c_str());
  }

  /**
   * @brief Handles BLE central disconnect events and resumes advertising.
   *
   * @param pServer Active NimBLE server instance.
   * @param connInfo Connection metadata from the disconnected central.
   * @param reason NimBLE disconnect reason code.
   */
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    (void)pServer;
    Serial.printf("BLE client disconnected (%d): %s\n", reason,
                  connInfo.getAddress().toString().c_str());
    NimBLEDevice::startAdvertising();
  }

  /**
   * @brief Provides the passkey used by secure BLE pairing flows.
   *
   * @return Configured six-digit passkey.
   */
  uint32_t onPassKeyDisplay() override {
    Serial.printf("BLE OTA passkey: %06lu\n", static_cast<unsigned long>(securityPasskey));
    return securityPasskey;
  }
} serverCallbacks;

class BleOtaCallbacks : public NimBLEOtaCallbacks {
 public:
  /**
   * @brief OTA start callback from NimBLEOta.
   *
   * @param ota OTA service instance.
   * @param firmwareSize Total firmware bytes expected.
   * @param reason Start reason emitted by the OTA protocol layer.
   */
  void onStart(NimBLEOta* ota, uint32_t firmwareSize, NimBLEOta::Reason reason) override {
    if (reason == NimBLEOta::Reconnected) {
      Serial.println("OTA client reconnected, resuming upload");
      ota->stopAbortTimer();
      return;
    }

    Serial.printf("OTA started (%s), size=%lu bytes\n", reasonToString(reason),
                  static_cast<unsigned long>(firmwareSize));
  }

  /**
   * @brief OTA progress callback with cumulative transferred bytes.
   *
   * @param ota OTA service instance.
   * @param current Number of bytes received so far.
   * @param total Total bytes expected for this firmware image.
   */
  void onProgress(NimBLEOta* ota, uint32_t current, uint32_t total) override {
    (void)ota;
    if (total == 0) {
      return;
    }

    Serial.printf("OTA progress: %.1f%% (%lu/%lu)\n",
                  (static_cast<float>(current) / static_cast<float>(total)) * 100.0f,
                  static_cast<unsigned long>(current), static_cast<unsigned long>(total));
  }

  /**
   * @brief OTA stop callback used to handle graceful interruption and aborts.
   *
   * @param ota OTA service instance.
   * @param reason Stop reason emitted by the OTA protocol layer.
   */
  void onStop(NimBLEOta* ota, NimBLEOta::Reason reason) override {
    if (reason == NimBLEOta::Disconnected) {
      Serial.println("OTA stopped because client disconnected, waiting 30s to resume");
      ota->startAbortTimer(30);
      return;
    }

    if (reason == NimBLEOta::StopCmd) {
      Serial.println("OTA stop command received, aborting current upload");
      ota->abortUpdate();
      return;
    }

    Serial.printf("OTA stopped, reason=%s\n", reasonToString(reason));
  }

  /**
   * @brief OTA completion callback that reboots into the new firmware image.
   *
   * @param ota OTA service instance.
   */
  void onComplete(NimBLEOta* ota) override {
    (void)ota;
    Serial.println("OTA completed successfully, rebooting in 2 seconds");
    delay(2000);
    ESP.restart();
  }

  /**
   * @brief OTA error callback used to log and abort failed transfers.
   *
   * @param ota OTA service instance.
   * @param err ESP-IDF error code.
   * @param reason High-level OTA failure reason.
   */
  void onError(NimBLEOta* ota, esp_err_t err, NimBLEOta::Reason reason) override {
    Serial.printf("OTA error: 0x%x (%s)\n", static_cast<unsigned int>(err), reasonToString(reason));
    ota->abortUpdate();
  }
} otaCallbacks;

}  // namespace

namespace ota {

/**
 * @brief Starts BLE OTA server, callbacks, and advertising.
 *
 * @param config OTA runtime configuration.
 * @return true when OTA is active or already active; false on startup failure.
 */
bool begin(const Config& config) {
  if (otaStarted) {
    return true;
  }

  securityPasskey = config.passkey;

  NimBLEDevice::init(config.deviceName);
  NimBLEDevice::setMTU(config.mtu);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks);

  if (config.secure) {
    NimBLEDevice::setSecurityPasskey(config.passkey);
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  }

  if (config.enableDeviceInfoService) {
    if (bleDis.init()) {
      bleDis.setManufacturerName(config.manufacturer);
      bleDis.setModelNumber(config.model);
      bleDis.setFirmwareRevision(config.firmwareVersion);
      bleDis.start();
    } else {
      Serial.println("Warning: Device Information Service failed to initialize");
    }
  }

  NimBLEService* otaService = bleOta.start(&otaCallbacks, config.secure);
  if (otaService == nullptr) {
    Serial.println("Failed to start BLE OTA service");
    return false;
  }

  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(bleOta.getServiceUUID());
  advertising->setMinInterval(0x06);
  advertising->setMaxInterval(0x12);
  advertising->start();

  healthCheckPending = isPartitionPendingVerify();
  if (healthCheckPending) {
    healthCheckStartMs = millis();
    Serial.printf("OTA health check pending (profile=%s, timeout=%lu ms)\n",
                  ota_cfg::kProfileName,
                  static_cast<unsigned long>(ota_cfg::kBootHealthcheckTimeoutMs));

    if (!internalStartupHealthChecks()) {
      failCurrentFirmwareAndRollback();
      return false;
    }
  }

  Serial.printf("BLE OTA ready: %s (profile=%s, secure=%s)\n", config.deviceName,
                ota_cfg::kProfileName, config.secure ? "true" : "false");
  otaStarted = true;
  return true;
}

/**
 * @brief Periodic OTA hook.
 *
 * The OTA stack is callback-driven and does not currently require polling.
 */
void loop() {
  if (isHealthCheckTimedOut()) {
    Serial.println("OTA health check timeout reached, rolling back");
    failCurrentFirmwareAndRollback();
  }
}

/**
 * @brief Indicates whether rollback health check is currently pending.
 *
 * @return true if image validation must still be completed.
 */
bool isHealthCheckPending() {
  return healthCheckPending;
}

/**
 * @brief Marks the currently running image as healthy if validation is pending.
 *
 * @return true if image is marked healthy or validation is not pending.
 */
bool confirmCurrentFirmwareHealthy() {
  if (!healthCheckPending) {
    return true;
  }

  esp_err_t state = esp_ota_mark_app_valid_cancel_rollback();
  if (state != ESP_OK) {
    Serial.printf("Failed to mark OTA image healthy: 0x%x\n",
                  static_cast<unsigned int>(state));
    return false;
  }

  healthCheckPending = false;
  Serial.println("OTA health check passed, rollback canceled");
  return true;
}

/**
 * @brief Marks running image invalid and triggers immediate rollback reboot.
 */
void failCurrentFirmwareAndRollback() {
  if (!healthCheckPending) {
    return;
  }

  Serial.println("Marking current OTA image invalid, rebooting to rollback");
  esp_err_t state = esp_ota_mark_app_invalid_rollback_and_reboot();
  if (state != ESP_OK) {
    Serial.printf("Rollback marking failed (0x%x), forcing reboot\n",
                  static_cast<unsigned int>(state));
    ESP.restart();
  }
}

/**
 * @brief Indicates whether OTA services have been initialized.
 *
 * @return true when OTA was started successfully.
 */
bool isStarted() {
  return otaStarted;
}

}  // namespace ota
