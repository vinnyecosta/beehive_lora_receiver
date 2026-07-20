# OTA Setup and Operation Guide

This guide explains how to configure, build, upload, and validate BLE OTA for
this project in both development and production profiles.

## 1) Prerequisites

- PlatformIO installed and working
- ESP32 board connected by USB for initial flashing
- BLE-capable phone or laptop
- OTA-capable client:
  - ESP BLE OTA App (official)
  - BLE web OTA tool (Chromium browser)

## 2) Files you should know

- `platformio.ini`:
  - Build profile selector and dependencies
- `include/ota_config.h`:
  - Profile defaults, passkey, secure mode, rollback thresholds
- `lib/ota_manager/src/OtaManager.h`:
  - OTA public API
- `lib/ota_manager/src/OtaManager.cpp`:
  - OTA behavior, callbacks, rollback logic
- `src/main.cpp`:
  - Application startup health confirmation
- `partitions_ota_ab.csv`:
  - A/B OTA partition layout

## 3) Choose profile (development or production)

Open `platformio.ini` and set:

```ini
build_flags =
  -DOTA_ACTIVE_PROFILE=OTA_PROFILE_PRODUCTION
```

Use one of:

- `OTA_PROFILE_DEV`
- `OTA_PROFILE_PRODUCTION`

### What changes between profiles

Defined in `include/ota_config.h`.

Typical differences:

- BLE secure mode (`kSecureBle`)
- BLE passkey (`kBlePasskey`)
- Device name (`kDeviceName`)
- MTU (`kMtu`)
- Rollback timeout (`kBootHealthcheckTimeoutMs`)

## 4) Configure passkey and security

In `include/ota_config.h`, set:

- `kSecureBle`
- `kBlePasskey`

Production profile currently defaults to secure BLE enabled with passkey and
MITM protection.

## 5) Build and first USB flash

From the project root:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

Why first USB flash is needed:

- It installs firmware that contains OTA service logic
- It ensures partition table is deployed with A/B layout

## 6) Verify OTA service startup logs

In serial monitor, confirm you see:

- OTA service ready log line
- Active profile in log
- Security mode indication

If this is first boot after an OTA update and rollback is pending, you should
also see health-check pending logs.

## 7) OTA update workflow (regular use)

### Option A: ESP BLE OTA App (official)

1. Build firmware:

```bash
pio run
```

2. Locate binary:

- `.pio/build/heltec_wifi_lora_32_V3/firmware.bin`

3. In app:

- Scan for device
- Connect to profile-specific device name
- Select firmware binary
- Start upload

4. Wait for device reboot and reconnection.

### Option B: Web BLE OTA Tool

1. Build firmware:

```bash
pio run
```

2. Open compatible BLE OTA web page in Chromium browser.
3. Connect to device.
4. Upload `.pio/build/heltec_wifi_lora_32_V3/firmware.bin`.
5. Wait for completion and reboot.

## 8) First boot after OTA: rollback health-check behavior

The image is not considered permanent immediately after OTA.

Sequence:

1. New image boots.
2. OTA module detects pending verification state.
3. Internal checks run (e.g., heap threshold).
4. Application checks run in `src/main.cpp` (radio startup path).
5. If healthy:
   - `ota::confirmCurrentFirmwareHealthy()` marks image valid.
6. If unhealthy or timed out:
   - `ota::failCurrentFirmwareAndRollback()` marks image invalid and reboots into previous image.

## 9) How to customize health-check policy

Change in `include/ota_config.h`:

- `kBootHealthcheckTimeoutMs`
- `kMinHealthyFreeHeapBytes`

Add app-specific checks in `src/main.cpp` before calling
`ota::confirmCurrentFirmwareHealthy()`.

Examples:

- Radio init success
- Sensor bus init success
- Cloud/LAN connectivity probe

## 10) Troubleshooting

### Device not visible in BLE scanner

- Confirm board booted successfully over serial
- Confirm profile device name in `include/ota_config.h`
- Reboot board and rescan

### Pairing fails

- Confirm passkey in `include/ota_config.h`
- Ensure production profile has secure mode enabled
- Clear previous bonding records on phone/client and retry

### OTA transfers but boot loops

- Watch serial logs for rollback health-check messages
- Check whether app startup fails before health confirmation call
- Increase health-check timeout for heavy startup paths

### Build fails due to profile macro

- Ensure `platformio.ini` uses one of:
  - `OTA_PROFILE_DEV`
  - `OTA_PROFILE_PRODUCTION`

## 11) Recommended production hardening

In addition to current implementation:

- Enable ESP secure boot
- Enable signed firmware verification
- Keep OTA passkey outside source control for production
- Add telemetry for OTA start/success/failure/rollback events
