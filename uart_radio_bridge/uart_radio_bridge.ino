/*
 * UART-to-nRF24 Packet Relay
 * This sketch reads bytes from the UART and relays them to a connected
 * nRF24L01 module and, ideally, a YT1500pro pan and tilt module.
 * You should make sure to update DEVICE_ID below to match the channel
 * you intend to use.
 */

#include <SPI.h>
#include <RF24.h>

#define CE_PIN   7
#define CSN_PIN  8

RF24 radio(CE_PIN, CSN_PIN);

const uint8_t RF_CHANNEL = 10;
const uint8_t DEVICE_ID = 0x05;
const uint8_t TARGET_ADDRESS[5] = {0x90, 0x70, 0xA1, 0x15, DEVICE_ID};

uint8_t payload[5];

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(F("--- UART Packet Relay Active ---"));

  if (!radio.begin()) {
    Serial.println(F("Radio hardware failure!"));
    while (1) {}
  }

  radio.setChannel(RF_CHANNEL);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.setCRCLength(RF24_CRC_16);
  radio.disableDynamicPayloads();
  radio.setPayloadSize(5);

  radio.openWritingPipe(TARGET_ADDRESS);
  radio.stopListening();

  Serial.println(F("Radio Configured. Waiting for 5-byte serial packets..."));
}

void loop() {
  if (Serial.available() >= 5) {
    Serial.readBytes(payload, 5);
    Serial.print(F("TX: [ "));
    for (int i = 0; i < 5; i++) {
      Serial.print(F("0x"));
      if (payload[i] < 16) Serial.print(F("0"));
      Serial.print(payload[i], HEX);
      Serial.print(F(" "));
    }
    Serial.print(F("] -> "));

    bool success = radio.write(&payload, 5);

    if (success) {
      Serial.println(F("ACK"));
    } else {
      Serial.println(F("No ACK"));
    }
  }
}
