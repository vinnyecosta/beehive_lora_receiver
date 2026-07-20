/**
 * 
 * FOR THIS EXAMPLE TO WORK, YOU MUST INSTALL THE "LoRaWAN_ESP32" LIBRARY USING
 * THE LIBRARY MANAGER IN THE ARDUINO IDE.
 * 
 * This code will send a two-byte LoRaWAN message every 15 minutes. The first
 * byte is a simple 8-bit counter, the second is the ESP32 chip temperature
 * directly after waking up from its 15 minute sleep in degrees celsius + 100.
 *
 * If your NVS partition does not have stored TTN / LoRaWAN provisioning
 * information in it yet, you will be prompted for them on the serial port and
 * they will be stored for subsequent use.
 *
 * See https://github.com/ropg/LoRaWAN_ESP32
*/


// Pause between sends in seconds, so this is every 15 minutes. (Delay will be
// longer if regulatory or TTN Fair Use Policy requires it.)
#define MINIMUM_DELAY 10 


#include <heltec_unofficial.h>
#include <LoRaWAN_ESP32.h>
#include <Preferences.h>
#include <RadioLib.h>
#include <OtaManager.h>
#include <payload.h>
LoRaWANNode* node;

RTC_DATA_ATTR uint8_t count = 0;

/**
 * @brief Saves LoRaWAN session state and enters deep sleep.
 *
 * The delay is selected as the greater of the project minimum delay and the
 * uplink duty-cycle delay required by the LoRaWAN stack.
 */
void goToSleep() {
  Serial.println("Going to deep sleep now");
  // allows recall of the session after deepsleep
//   TODO This is used to wipeout the nvm data
//   persist.wipe();   // same effect as pref.clear() for the "lorawan" ns
//     ESP.restart();
  if (node != nullptr) {
    persist.saveSession(node);
  }
  // Calculate minimum duty cycle delay (per FUP & law!)
  uint32_t interval = 0;
  if (node != nullptr) {
    interval = node->timeUntilUplink();
  }
  // And then pick it or our MINIMUM_DELAY, whichever is greater
  uint32_t delayMs = max(interval, (uint32_t)MINIMUM_DELAY * 1000);
  Serial.printf("Next TX in %i s\n", delayMs/1000);
  delay(100);  // So message prints
  // and off to bed we go
  heltec_deep_sleep(delayMs/1000);
}

PayloadDecoded receivedData;
uint8_t receivedBuffer[kMaxPayloadLengthV1];
// SensorData receivedData;
volatile bool receivedFlag = false;

void setFlag(void) {
    receivedFlag = true;
}

/**
 * @brief Initializes board peripherals, BLE OTA service, and LoRa radio.
 *
 * OTA is configured first so firmware updates are possible without modifying
 * the application logic in this file.
 */
void setup() {
  heltec_setup();

  ota::begin();

  // Obtain directly after deep sleep
  // May or may not reflect room temperature, sort of. 
  float temp = heltec_temperature();
  Serial.printf("Temperature: %.1f °C\n", temp);

  // initialize radio
  Serial.println("Radio init");
  int16_t state = radio.begin(915.0, 125.0, 7, 5, 0x12, 10, 8, 1.6, false);
//   int16_t state = radio.begin(915.0, 250.0, 7, 5, 0x12, 20, 10, 0, false);
    if (state == RADIOLIB_ERR_NONE) 
        Serial.println("LoRa initialized");
    else {
        Serial.println("Radio did not initialize. We'll try again later.");
        ota::failCurrentFirmwareAndRollback();
        goToSleep();
    }

  if (!ota::confirmCurrentFirmwareHealthy()) {
    ota::failCurrentFirmwareAndRollback();
  }

//   node = persist.manage(&radio);

//   if (!node->isActivated()) {
//     Serial.println("Could not join network. We'll try again later.");
//     goToSleep();
//   }

  // If we're still here, it means we joined, and we can send something

  // Manages uplink intervals to the TTN Fair Use Policy
//   node->setDutyCycle(true, 1250);

//   uint8_t uplinkData[2];
//   uplinkData[0] = count++;
//   uplinkData[1] = temp + 100;

//   uint8_t downlinkData[256];
//   size_t lenDown = sizeof(downlinkData);

//   state = node->sendReceive(uplinkData, sizeof(uplinkData), 1, downlinkData, &lenDown);

//   if(state == RADIOLIB_ERR_NONE) {
//     Serial.println("Message sent, no downlink received.");
//   } else if (state > 0) {
//     Serial.println("Message sent, downlink received.");
//   } else {
//     Serial.printf("sendReceive returned error %d, we'll try again later.\n", state);
//   }

//   goToSleep();    // Does not return, program starts over next round

// Set the callback function for received packets
    radio.setDio1Action(setFlag);

    // Start receiving indefinitely
    state = radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF); 
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Started receiving!");
    }

    Serial.printf("Header size: %u, Sample size: %u, min packet: %u, max packet: %u\n",
                  sizeof(PayloadHeaderV1), sizeof(SamplePayloadV1),
                  sizeof(PayloadHeaderV1) + sizeof(SamplePayloadV1) + sizeof(uint16_t),
                  kMaxPayloadLengthV1);
}




/**
 * @brief Main firmware loop.
 *
 * Keeps OTA event integration point available and waits for LoRa packets,
 * printing the payload and RSSI when reception succeeds.
 */
void loop() {
  // This is never called. There is no repetition: we always go back to
  // deep sleep one way or the other at the end of setup()
  ota::loop();

  // Interrupt-driven receive: the DIO1 callback sets receivedFlag when a
  // packet arrives. Do NOT use radio.receive() here; startReceive() is
  // already running in the background.
  if (receivedFlag) {
    receivedFlag = false;

    // Query the actual received packet length, then read it into our buffer.
    size_t len = radio.getPacketLength();
    if (len > sizeof(receivedBuffer)) {
      Serial.printf("Packet too large (%u bytes), truncating to %u\n", len,
                    sizeof(receivedBuffer));
      len = sizeof(receivedBuffer);
    }
    int state = radio.readData(receivedBuffer, len);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.printf("Received %u bytes:", len);
      for (size_t i = 0; i < len; ++i) {
        Serial.printf(" %02X", receivedBuffer[i]);
      }
      Serial.println();

      Status parseStatus = DeserializePayloadV1(receivedBuffer, len, &receivedData);
      if (parseStatus == Status::kOk) {
        // temperatureCentiC is in centi-degrees, divide by 100.0 for °C.
        Serial.printf("Packet ID: %d, Temp: %.2f C, RSSI: %d, SNR: %.2f\n",
                      receivedData.header.nodeId,
                      receivedData.currentSample.temperatureCentiC / 100.0f,
                      receivedData.header.rssiDbm,
                      receivedData.header.snrQuarterDb / 4.0f);
      } else {
        Serial.printf("Payload decode failed, status %d\n", static_cast<int>(parseStatus));
      }
    } else {
      Serial.printf("Failed to read packet, code %d\n", state);
    }

    // Go back to listening for the next packet
    radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  }
}
