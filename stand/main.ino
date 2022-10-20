#include "HX711.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

int red_light_pin = 5;
int green_light_pin = 6;
int blue_light_pin = 4;
int ignite_relay_pin = 11;

String incommingPackage;

const int DOUT = A1;
const int CLK = A0;

int countDown = 60;

long unsigned lastUpd = millis();
int state = 0;

HX711 balanza;
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

void setRGBColor(int red_light_value, int green_light_value, int blue_light_value)
{
  analogWrite(red_light_pin, red_light_value);
  analogWrite(green_light_pin, green_light_value);
  analogWrite(blue_light_pin, blue_light_value);
}

void sendPackage(String value)
{
  Serial.println(String(millis()) + "|" + String(state) + "|" + value);
}

void printValue()
{
  const int m = balanza.get_units(1) * -1;
  updateLCD(m);
  sendPackage(String(m));
}

String getPrintable(int m)
{
  const String str = String(m);
  String returnableStr = str;

  for (int i = returnableStr.length(); i < 16; i++)
  {
    returnableStr += " ";
  }
  return returnableStr;
}

bool asyncDelay(int delayTime, void (*cb)())
{
  long unsigned startingTime = millis();
  while (millis() < startingTime + delayTime)
  {
    (*cb)();
  }
}

void startIgnitionSequence()
{
  digitalWrite(ignite_relay_pin, HIGH);
  sendPackage("HIGH");
  asyncDelay(3000, printValue);
  digitalWrite(ignite_relay_pin, LOW);
  sendPackage("LOW");
}

void updateLCD(int m)
{
  if (abs(millis() - lastUpd) > 250)
  {
    const String printable = getPrintable(m);
    lcd.setCursor(0, 1);
    lcd.print(printable);
    lastUpd = millis();
  }
}

void abort()
{
  state = -1;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ABORTED!");
  setRGBColor(255, 255, 255);
  while (true)
  {
    sendPackage("ABORTED");
  }
}

// ---- MAIN ----

void setup()
{
  pinMode(red_light_pin, OUTPUT);
  pinMode(green_light_pin, OUTPUT);
  pinMode(blue_light_pin, OUTPUT);
  pinMode(ignite_relay_pin, OUTPUT);

  setRGBColor(255, 0, 255);

  lcd.init();
  lcd.backlight();
  lcd.print("Calibrating...");
  Serial.begin(9600);
  balanza.begin(DOUT, CLK);
  balanza.read();
  balanza.set_scale(-202.5); // Establecemos la escala
  balanza.tare(20);          // El peso actual es considerado Tara.
  lcd.setCursor(0, 1);
  lcd.print("Completed!");
  lcd.setCursor(0, 0);
  delay(2000);
  lcd.clear();
}

void loop()
{
  if (Serial.available() > 0)
  {
    incommingPackage = Serial.readString();
    if (incommingPackage.endsWith("\n"))
    {
      incommingPackage = incommingPackage.substring(0, incommingPackage.length() - 1);
    }
    if (incommingPackage == "ABORT")
    {
      abort();
    }
  }

  switch (state)
  {
  case 0:
    sendPackage("awaiting");
    lcd.print("Awaiting");
    lcd.setCursor(0, 1);
    lcd.print("Start signal");

    if (incommingPackage != "GO")
      return;

    lcd.clear();
    state = 1;
    break;
  case 1:
    lcd.print("Final countdown:");
    setRGBColor(0, 255, 0);
    if (countDown > 0)
    {
      countDown--;
      updateLCD(countDown);
      sendPackage(String(countDown));
      delay(1000);
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
