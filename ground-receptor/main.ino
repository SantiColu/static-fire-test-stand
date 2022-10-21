#include <SPI.h>

#include "RF24.h"
#include "nRF24L01.h"

const uint64_t CE_PIN = 7;
const uint64_t CSN_PIN = 8;
const uint64_t NOSE_PIN = 10;

RF24 radio(CE_PIN, CSN_PIN);
const uint64_t pipes[2] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};

const uint64_t SERIAL_RATE = 9600;

void setup() {
  pinMode(NOSE_PIN, OUTPUT);
  Serial.begin(SERIAL_RATE);
  radio.begin();
  radio.setRetries(254, 254);  // Maximos reintentos
  // radio.setPayloadSize(8);    // Reduce el payload de 32 si tienes problemas
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
}

void loop() {
  radio.startListening();
  if (radio.available()) {
    char text[32] = "";
    radio.read(&text, sizeof(text));
    Serial.println(text);
  }

  if (Serial.available()) {
    radio.stopListening();
    delay(100);
    String data = Serial.readString();

    char msg[data.length() + 1];
    data.toCharArray(msg, data.length() + 1);

    bool ok = radio.write(&msg, sizeof(msg));
    if (ok) {
      Serial.println("internal|success|" + data);
    } else {
      // TODO: SI HAY UN ERROR, SPAMEAR ABORT
      Serial.println("internal|error|" + data);
    }
  }
}
