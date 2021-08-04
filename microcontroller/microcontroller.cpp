#include <Arduino.h>

constexpr uint8_t version = 0x02;

typedef struct state_flags_t {
  bool receiving_data  = false;
  bool receiving_flags = false;
  bool sending         = false;
  bool high            = false;
  bool low             = false;

  uint8_t recv_size           = 0x00;
  uint8_t recv_buff_pos       = 0x00;
  uint8_t recv_buff_high[256] = {};
  uint8_t recv_buff_low[256]  = {};

  uint8_t send_size           = 0x00;
  uint8_t send_buff_pos       = 0x00;
  uint8_t send_buff_high[256] = {};
  uint8_t send_buff_low[256]  = {};
} state_flags_t;

state_flags_t state;

void setup() {
  Serial.begin(9600);
  Serial.write(0x01);
  Serial.write(version);
}

void panic() {
  cli();

  while (true) {
    __asm__("nop\n\t");
  }
}

void loop() {
  if (state.sending) {
    // TODO: Send correct data
    Serial.write(0x08);
    Serial.write(0xFF);

    uint8_t i = 0x00;

    do {
      Serial.write(i);
      Serial.write(0xFF - i);
    } while (i++ < 0xFF);

    state.sending = false;
  }

  if (Serial.available() > 1) {
    uint8_t data_high = 0x00;
    uint8_t data_low  = 0x00;

    Serial.readBytes(&data_high, 1);
    Serial.readBytes(&data_low, 1);

    if (state.receiving_data) {
      // TODO: Write this data to EEPROM
      state.recv_buff_high[state.recv_buff_pos] = data_high;
      state.recv_buff_low[state.recv_buff_pos]  = data_low;

      if (state.rec_size == 0) {
        state.recv_buff_pos  = 0;
        state.receiving_data = false;

        Serial.write(0x07);
        Serial.write(0xFF);  // TODO: Check number of correct words sent
      } else {
        state.rec_size--;
        state.recv_buff_pos++;
      }

    } else if (state.receiving_flags) {
      switch (state.recv_buff_pos) {
        case 0x00:
          state.high = data_high;
          state.low  = data_low;

          state.rec_size--;
          state.recv_buff_pos++;
          break;

        case 0x01:
          state.sending = data_high;

          state.rec_size      = 0;
          state.recv_buff_pos = 0;

          Serial.write(0x05);
          Serial.write(0x01);

          state.receiving_flags = false;
          break;

        default:
          Serial.write(0x03);
          Serial.write(0x02);

          panic();

          state.rec_size      = 0;
          state.recv_buff_pos = 0;
          break;
      }

    } else {
      if (data_high == 0x02) {
        state.receiving_flags = true;
        state.rec_size        = data_low;

      } else if (data_high == 0x06) {
        state.receiving_data = true;
        state.rec_size       = data_low;

      } else {
        Serial.write(0x03);
        Serial.write(0x01);

        panic();
      }
    }
  }
}
