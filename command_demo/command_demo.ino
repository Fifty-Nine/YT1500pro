/*
 * YT1500pro opcode demo
 * This demo cycles through all the known opcodes for the device
 * and demonstrates their functionality. If you want to use this,
 * you need an nRF24L01 module hooked up to your MCU and you'll
 * need to update the call to pantilt.begin() to pass the channel
 * number you intend to use (it should match the first two digits
 * of the seven segment display on your device).
 */
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 7
#define CSN_PIN 8

enum class YT1500ProOpcode : uint8_t
{
  TILT_UP = 0x1f,
  TILT_DOWN = 0x21,
  PAN_RIGHT = 0x23,
  PAN_LEFT = 0x25,
  MOVE_POSITION_A = 0x29,
  MOVE_POSITION_B = 0x2b,
  CYCLE_POSITIONS = 0x2d,
  STOP = 0x2f,
  SET_POSITION_A = 0x43,
  SET_POSITION_B = 0x45
};

void dumpPacket(const uint8_t *packet, size_t size)
{
  for (size_t i = 0; i < size; ++i) {
    if (i != 0) Serial.print(", ");
    Serial.print(packet[i], HEX);
  }
}

class YT1500Pro
{
public:
  YT1500Pro(rf24_gpio_pin_t cePin, rf24_gpio_pin_t csPin) :
    radio { cePin, csPin },
    channel { 0 }
  {

  }

  bool begin(uint8_t channel)
  {
    if (!radio.begin()) return false;
    radio.setChannel(10);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_LOW);
    radio.setCRCLength(RF24_CRC_16);
    radio.disableDynamicPayloads();
    radio.setPayloadSize(5);
    setChannel(channel);

    return true;
  }

  bool setChannel(uint8_t channel)
  {
    this->channel = channel;

    uint8_t address[] = { 0x90, 0x70, 0xa1, 0x15, channel };
    radio.openWritingPipe(address);
    radio.stopListening();

    return true;
  }

  void sendCommand(uint8_t opcode, uint8_t panSpeed = 8, uint8_t tiltSpeed = 8)
  {
    uint8_t packet[] = { channel, opcode, panSpeed, tiltSpeed, 0xa1 };

    Serial.print("TX [ ");
    dumpPacket(packet, 5);
    Serial.println("]");
    if (!radio.write(&packet, 5)) {
      Serial.print("radio nacked packet with opcode ");
      Serial.println(opcode, HEX);
    }
  }

  void sendCommand(YT1500ProOpcode opcode, uint8_t panSpeed = 8, uint8_t tiltSpeed = 8)
  {
    sendCommand((uint8_t)opcode, panSpeed, tiltSpeed);
  }

  void panLeft(uint8_t speed = 8) {
    sendCommand(YT1500ProOpcode::PAN_LEFT, speed);
  }

  void panRight(uint8_t speed = 8) {
    sendCommand(YT1500ProOpcode::PAN_RIGHT, speed);
  }

  void tiltUp(uint8_t speed = 8) {
    sendCommand(YT1500ProOpcode::TILT_UP, 8, speed);
  }

  void tiltDown(uint8_t speed = 8) {
    sendCommand(YT1500ProOpcode::TILT_DOWN, 8, speed);
  }

  void panToA(uint8_t speed = 8) {
    sendCommand(YT1500ProOpcode::MOVE_POSITION_A, speed);
  }

  void panToB(uint8_t speed = 8) {
    sendCommand(YT1500ProOpcode::MOVE_POSITION_B, speed);
  }

  void stop() {
    sendCommand(YT1500ProOpcode::STOP, 8, 8);
  }

  void setPositionA() {
    sendCommand(YT1500ProOpcode::SET_POSITION_A);
  }

  void setPositionB() {
    sendCommand(YT1500ProOpcode::SET_POSITION_B);
  }

  void cycleMarks(uint8_t speed = 8)
  {
    sendCommand(YT1500ProOpcode::CYCLE_POSITIONS, speed);
  }

  bool setupFailed() const {
    return failed;
  }

private:
  RF24 radio;
  uint8_t channel;
  bool failed = false;
};

YT1500Pro pantilt { CE_PIN, CSN_PIN };

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println(F("Starting up"));

  if (!pantilt.begin(5)) {
    Serial.println(F("YT1500 init failed"));
    while (1) { }
  }
}

void loop() {
  Serial.println("Waiting to send.");


  while (Serial.read() == -1) {}
  while (Serial.read() != -1) {}

  struct TableEntry {
    const char *name;
    void (*fn)(void);
    unsigned delay;
  };

  TableEntry table[] = {
    { "tilt down", []() { pantilt.tiltDown(); }, 2000 },
    { "tilt up",   []() { pantilt.tiltUp(); }, 2000 },
    { "pan right", []() { pantilt.panRight(); }, 3000 },
    { "set A",     []() { pantilt.setPositionA(); }, 400 },
    { "pan left",  []() { pantilt.panLeft(); }, 4600 },
    { "set B",     []() { pantilt.setPositionB(); }, 400 },
    { "cycle A-B", []() { pantilt.cycleMarks(); }, 20000 }
  };

  while (Serial.available() == 0) {
  for (size_t i = 0; i < sizeof(table) / sizeof(TableEntry); ++i) {
    auto& entry = table[i];
    Serial.print("Testing ");
    Serial.println(entry.name);
    entry.fn();
    delay(entry.delay);
    pantilt.stop();
    delay(1000);
  }
  }

  while (Serial.read() != -1) {}
}