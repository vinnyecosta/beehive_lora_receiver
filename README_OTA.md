# Modular OTA (BLE) for working_heltec_receiver

This project uses a dedicated OTA module and supports BLE OTA uploads with safer
boot behavior.

## Key files

- OTA module implementation: `lib/ota_manager/src/OtaManager.cpp`
- OTA module API: `lib/ota_manager/src/OtaManager.h`
- OTA profile and passkey config header: `include/ota_config.h`
- Main integration call site: `src/main.cpp`
- A/B partition table: `partitions_ota_ab.csv`
- Build profile selector: `platformio.ini`
- Additional setup walkthrough: `README_OTA_SETUP.md`

## What is implemented now

1. Modular OTA architecture with OTA logic isolated from `main.cpp`.
2. Secure BLE mode enabled by default in production profile:
   - Passkey pairing
   - MITM protection
3. Passkey and OTA behavior exposed in `include/ota_config.h`.
4. Compile-time OTA profile switch (development vs production).
5. First-boot rollback health-check flow for new OTA images.
6. A/B app partitions (`ota_0`, `ota_1`) for safer OTA deployments.

## Build, flash, monitor

Run these commands from project root:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## OTA profile switch (compile time)

Profile is selected at compile time in `platformio.ini`:

```ini
build_flags =
  -DOTA_ACTIVE_PROFILE=OTA_PROFILE_PRODUCTION
```

Available values:

- `OTA_PROFILE_DEV`
- `OTA_PROFILE_PRODUCTION`

You can switch profile by changing the `build_flags` value and rebuilding.

## Passkey and secure BLE defaults

All security defaults are centralized in `include/ota_config.h`.

Important fields:

- `kSecureBle`
- `kBlePasskey`
- `kBootHealthcheckTimeoutMs`
- `kMinHealthyFreeHeapBytes`

Production profile defaults:

- `kSecureBle = true`
- Passkey + MITM enabled
- Higher MTU for OTA throughput

Development profile defaults:

- `kSecureBle = false`
- Lower-friction local testing behavior

## Rollback health-check flow on first boot after OTA

The flow is now:

1. Device boots into new OTA image.
2. OTA module detects `ESP_OTA_IMG_PENDING_VERIFY` state.
3. Internal lightweight checks run (for example heap threshold).
4. Application startup checks run in `main.cpp`.
5. If app startup is healthy, call `ota::confirmCurrentFirmwareHealthy()`.
6. If startup fails or timeout expires, call `ota::failCurrentFirmwareAndRollback()` or automatic timeout rollback triggers.

This prevents broken OTA images from being permanently accepted.

## Main integration behavior

`src/main.cpp` now does only what is necessary:

- `ota::begin()` during startup
- `ota::confirmCurrentFirmwareHealthy()` after radio init succeeds
- `ota::failCurrentFirmwareAndRollback()` if radio init fails
- `ota::loop()` in the main loop for timeout-based rollback checks

## OTA upload tools

### Web-based BLE OTA tool

1. Build firmware and locate binary:
   - `.pio/build/heltec_wifi_lora_32_V3/firmware.bin`
2. Open a BLE OTA web tool in a Chromium-based browser.
3. Connect to the OTA device name from active profile.
4. Select firmware binary and start transfer.
5. Wait for completion and reboot.

### ESP BLE OTA App (official)

1. Build firmware: `pio run`
2. Open ESP BLE OTA app.
3. Scan and connect to the board.
4. Select firmware binary.
5. Start OTA and wait for reboot.

## Best-practice status

### A/B + robust boot + cryptographic verification

- A/B partition scheme is implemented.
- Rollback health-check is implemented.
- Still recommended for production hardening:
  - Secure boot enablement
  - Signed firmware verification

### BLE performance tuning

- Profile defaults include MTU tuning.
- Keep tuning connection interval and PHY based on your phone/client behavior and RF environment.

### Dedicated BLE OTA solution

- Implemented with `h2zero/NimBLEOta`.

## Operational note

If you need a full setup and day-to-day workflow guide (including profile edits,
pairing flow, and troubleshooting), read `README_OTA_SETUP.md`.
