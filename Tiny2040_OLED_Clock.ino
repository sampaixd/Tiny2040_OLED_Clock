#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/dtostrf.h>
#include "RTClib.h"

#define screenHeight 64
#define screenWidth 128
#define resetPin -1
#define screenAddress 0x3C
#define timeDelay 1000

Adafruit_SSD1306 display(screenWidth, screenHeight, &Wire, resetPin);
RTC_DS3231 rtc;

char daysOfWeek[7][12] = { 
 "Sunday",
 "Monday",
 "Tuesday",
 "Wednesday",
 "Thursday",
 "Friday",
 "Saturday"
};

const int PWMPin = A0;
const int buttonPin = 7;

int PWMValue = analogRead(PWMPin);
int prevPWMValue = PWMValue; // used to compare current and past, used for changing slides on the clock
int currentSlide = 0; // defines where you are on the clock currently
int prevSecond = 0; //used to check if the clock should update or not

bool changedSlide = true;

void prepareDisplayChars() {
 display.setTextSize(2);
 display.setTextColor(SSD1306_WHITE);
 display.setCursor(10, 10);
 display.cp437(true);
 }

int getCurrentTime(DateTime now){
  String currentTime = String();
  if (now.hour() < 10)
  {
    currentTime += "0";
    currentTime += now.hour();
    Serial.print("0");
    Serial.print(now.hour());
  }
    else
  {
    currentTime += now.hour();
    Serial.print(now.hour(), DEC);
  }
  currentTime += ":";
  Serial.print(":");
  
  if (now.minute() < 10)
  {
    currentTime += "0";
    currentTime += now.minute();
    Serial.print("0");
    Serial.print(now.minute());
  }
  else
  {
    currentTime += now.minute();
    Serial.print(now.minute(), DEC);
  }
  currentTime += ":";
  Serial.print(":");

  if (now.second() < 10)
  {
    currentTime += "0";
    currentTime += now.second();
    Serial.print("0");
    Serial.println(now.second());
  }
  else
  {
    currentTime += now.second();
    Serial.println(now.second(), DEC);
  }
  Serial.println(currentTime);
  char currentTimeChar[10]; 
  currentTime.toCharArray(currentTimeChar, 10);
  currentTime = "";
  display.write(currentTimeChar);
  return 0;
}

void clockDisplay(DateTime now)
{
  getCurrentTime(now);    
    display.write("\n");
    //display.write(daysOfWeek[now.dayOfTheWeek()]);
    char dateChar[8];
    sprintf(dateChar, " %d/%d", now.day(), now.month());
    //Serial.println(dateChar);
    display.write(dateChar);
    display.write("\n");
    char temperature[12];
    display.write(dtostrf(rtc.getTemperature(), 6, 2, temperature));
    display.write(" C");
    //Serial.print("Temperature: ");
    //Serial.print(("%.2f", rtc.getTemperature()));
    //Serial.println(" C");
    display.display();
    //delay(timeDelay);
    //display.clearDisplay();
    display.setCursor(10, 10);
}
 
void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, screenAddress);
  display.clearDisplay();
  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // used when you need to resync clock
  prepareDisplayChars();
}

void loop() {
  DateTime now = rtc.now();
  PWMValue = analogRead(PWMPin);
  Serial.println(PWMValue, DEC);
  Serial.println(currentSlide, DEC);
  if (PWMValue >= prevPWMValue + 10)
  {
    prevPWMValue = PWMValue;
    if (currentSlide < 1) // max value
    {
      currentSlide++;
      display.clearDisplay();
      display.setCursor(10, 10);
      changedSlide = false;
    }
  }

  else if (PWMValue <= prevPWMValue - 10)
  {
    prevPWMValue = PWMValue;
    if (currentSlide > 0) // min value
    {
      currentSlide--;
      display.clearDisplay();
      display.setCursor(10, 10);
      changedSlide = false;
    }
  }
  
  if (now.second() != prevSecond && currentSlide == 0)
  {
    display.clearDisplay();
    clockDisplay(now);
    prevSecond = now.second();
  }
  else if (currentSlide == 1 && !changedSlide)
  {
    display.write("sample text");
    display.display();
    changedSlide = true;
  }
}
