#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>

#include "HX711.h"
#include "RF24.h"
#include "nRF24L01.h"

const int red_light_pin = 5;
const int green_light_pin = 6;
const int blue_light_pin = 4;
const int ignite_relay_pin = 2;

const uint64_t CE_PIN = 7;
const uint64_t CSN_PIN = 8;
const uint64_t NOSE_PIN = 10;

RF24 radio(CE_PIN, CSN_PIN);
const uint64_t pipes[2] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};

String incommingPackage;

const int DOUT = A1;
const int CLK = A0;

int countDown = 60;

long unsigned lastUpd = millis();
int state = 0;

HX711 balanza;
LiquidCrystal_I2C lcd(
    0x27, 16,
    2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setRGBColor(int red_light_value, int green_light_value,
                 int blue_light_value) {
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

void sendPackage(String value) {
  String text = String(millis()) + "|" + String(state) + "|" + value;

  char msg[text.length() + 1];
  text.toCharArray(msg, text.length() + 1);

  bool ok = radio.write(&msg, sizeof(msg));

  if (ok)
    Serial.print("Package sended: ");
  else
    Serial.print("Package failed: ");

  Serial.println(msg);
}

void printValue() {
  const int m = balanza.get_units(1) * -1;
  updateLCD(m);
  sendPackage(String(m));
}

String getPrintable(int m) {
  const String str = String(m);
  String returnableStr = str;

  for (int i = returnableStr.length(); i < 16; i++) {
    returnableStr += " ";
  }
  return returnableStr;
}

bool asyncDelay(int delayTime, void (*cb)()) {
  long unsigned startingTime = millis();
  while (millis() < startingTime + delayTime) {
    (*cb)();
  }
}

void startIgnitionSequence() {
  digitalWrite(ignite_relay_pin, HIGH);
  sendPackage("HIGH");
  asyncDelay(3000, printValue);
  digitalWrite(ignite_relay_pin, LOW);
  sendPackage("LOW");
}

void updateLCD(int m) {
  if (abs(millis() - lastUpd) > 250) {
    const String printable = getPrintable(m);
    lcd.setCursor(0, 1);
    lcd.print(printable);
    lastUpd = millis();
  }
}

void abort() {
  state = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ABORTED!");
  setRGBColor(255, 255, 255);
  sendPackage("ABORTED");
  while (true) {
    sendPackage("ABORTED");
  }
}

// ---- MAIN ----

void setup() {
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
  pinMode(ignite_relay_pin, OUTPUT);

  setRGBColor(255, 0, 255);

  radio.begin();
  radio.setRetries(15, 5);  // Maximos reintentos
  // radio.setPayloadSize(8);    // Reduce el payload de 32 si tienes problemas
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);

  lcd.init();
  lcd.backlight();
  lcd.print("Calibrating...");
  Serial.begin(9600);
  balanza.begin(DOUT, CLK);
  balanza.read();
  balanza.set_scale(-202.5);  // Establecemos la escala
  balanza.tare(20);           // El peso actual es considerado Tara.
  lcd.setCursor(0, 1);
  lcd.print("Completed!");
  lcd.setCursor(0, 0);
  delay(2000);
  lcd.clear();
}

void listenRadio(int t) {
  radio.startListening();  // Volvemos a la escucha
  unsigned long started_waiting_at = millis();

  while (!radio.available()) {
    if (millis() - started_waiting_at > t) {
      return;
    }
  }

  char text[32] = "";
  radio.read(&text, sizeof(text));
  if (text != "") {
    // Serial.print("Mensaje recibido: - ");
    // Serial.print(sizeof(text));
    // Serial.print(" -");
    // Serial.println(text);
    incommingPackage = String(text);
    if (incommingPackage.endsWith("\n")) {
      incommingPackage =
          incommingPackage.substring(0, incommingPackage.length() - 1);
    }
    if (incommingPackage == "ABORT") {
      delay(100);
      radio.stopListening();
      abort();
    }
  }
}

void loop() {
  if (state < 2) {
    listenRadio(200);
  }

  radio.stopListening();
  switch (state) {
    case 0:
      sendPackage("awaiting");
      lcd.print("Awaiting");
      lcd.setCursor(0, 1);
      lcd.print("Start signal");

      if (incommingPackage != "GO") return;

      lcd.clear();
      state = 1;
      break;
    case 1:
      lcd.print("Final countdown:");
      setRGBColor(0, 255, 0);
      if (countDown > 0) {
        countDown--;
        updateLCD(countDown);
        sendPackage(String(countDown));
        listenRadio(800);
        return;
      }
      state = 2;
      break;
    case 2:
      lcd.clear();
      lcd.print("Value:");
      setRGBColor(255, 255, 0);
      asyncDelay(5000, printValue);
      startIgnitionSequence();
      state = 3;
      break;
    case 3:
      setRGBColor(255, 0, 0);
      printValue();
      break;
  }
}