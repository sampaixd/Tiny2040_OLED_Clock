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

const int PWMPin = A0;
const int buttonPin = 7;

bool sleepMode = false; // my attempt at saving power

int buttonState = 0;
int PWMValue = analogRead(PWMPin);
int prevPWMValue = PWMValue; // used to compare current and past, used for changing slides or selected content on the clock
int currentSlide = 0; // defines where you are on the clock currently

// variables related to the alarm function
int alarmClock[3] = {0, 0, 0};  // hour, minute, second
int prevSecond = 0; //used to check if the alarm should update or not
bool alarmActive = false;
bool alarmTriggered = false;


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

void resetDisplay()
{
    display.clearDisplay();
    display.setCursor(10, 10);
}

void clockDisplay(DateTime now)
{
  getCurrentTime(now);    
    display.write("\n");
    char dateChar[8];
    sprintf(dateChar, " %d/%d", now.day(), now.month());
    display.write(dateChar);
    display.write("\n");
    char temperature[12];
    display.write(dtostrf(rtc.getTemperature(), 6, 2, temperature));
    display.write(" C");
    display.display();
}

void alarmTriggeredDisp()
{
  ulong prevMillis = millis();
  resetDisplay();
  while (buttonState == LOW)
  {
    if (millis() - prevMillis >= 2000)
    {
      display.invertDisplay(false);
      prevMillis = millis();
    }
    else if (millis() - prevMillis >= 1000)
    {
      display.invertDisplay(true);
    }
    resetDisplay();
    display.display();
    buttonState = digitalRead(buttonPin);
  }
  display.invertDisplay(false);
  alarmTriggered = false;
  alarmActive = false;
}

void decrementAlarmTimer()
{
  if (alarmClock[0] <= 0 && alarmClock[1] <= 0 && alarmClock[2] <= 1)
  {
    alarmClock[2]--;
    alarmTriggered = true;
  }
  else if (alarmClock[2] <= 0 && alarmClock[1] <= 0)
  {
    alarmClock[2] = 59;
    alarmClock[1] = 59;
    alarmClock[0]--;
  }
  else if (alarmClock[2] <= 0)
  {
    alarmClock[2] = 59;
    alarmClock[1]--;
  }
  else
  {
    alarmClock[2]--;
  }
}

void alarmClockManager(DateTime now)
{
  if (prevSecond != now.second())
  {
    decrementAlarmTimer();
    prevSecond = now.second();
  }
}

int getPWMValue(int currentOption, int maxValue)
{
  PWMValue = analogRead(PWMPin);
  //Serial.println(PWMValue, DEC);
  if (PWMValue >= prevPWMValue + 20)
  {
    prevPWMValue = PWMValue;
    if (currentOption < maxValue)
      return ++currentOption;
    return 0;
  }

  else if (PWMValue <= prevPWMValue - 20)
  {
    prevPWMValue = PWMValue;
    if (currentOption > 0) // min value
      return --currentOption;
    return maxValue;
  }
  return currentOption;
}



void writeAlarmDisplay(int currentClockSelected)
{
  char clockChars[7];
  sprintf(clockChars, "%d%d%d", alarmClock[0], alarmClock[1], alarmClock[2]);
  display.write("Alarm\n");
  int ammountBelow10 = 0; // 
  for (int i = 0; i < 6; i++)
  {
    if (i % 2 == 0 && i > 0)
      display.write(":");
      
    if (currentClockSelected == i)
    {
      display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }
    if (alarmClock[i / 2] < 10 && i % 2 == 0)
    {
      display.write("0");
      ammountBelow10++;
    }
    else
      display.write(clockChars[i - ammountBelow10]);
    display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
  display.display();
}

void alarmCheckValueBoundaries()
{
  if (alarmClock[0] >= 90)
    alarmClock[0] -= 70;
  else if (alarmClock[0] >= 30)
    alarmClock[0] -= 30;
  else if (alarmClock[0] >= 24)
    alarmClock[0] -= 20;

  if (alarmClock[1] >= 90)
    alarmClock[1] -= 40;
  else if (alarmClock[1] >= 60)
    alarmClock[1] -= 60;

  if (alarmClock[2] >= 90)
    alarmClock[2] -= 40;
  else if (alarmClock[2] >= 60)
    alarmClock[2] -= 60;
}

void editAlarm(int currentClockSelected)
{
 while (buttonState == LOW)
 { 
    if (currentClockSelected % 2 == 1)
      alarmClock[currentClockSelected / 2] = getPWMValue(alarmClock[currentClockSelected / 2] % 10, 9) + (alarmClock[currentClockSelected / 2] / 10) * 10;
    else
      alarmClock[currentClockSelected / 2] = 10 * getPWMValue(alarmClock[currentClockSelected / 2] / 10, 9) + alarmClock[currentClockSelected / 2] % 10;

    alarmCheckValueBoundaries();
      
    resetDisplay();
    writeAlarmDisplay(currentClockSelected);
    buttonState = digitalRead(buttonPin);
    
 }
 delay(200);
 resetDisplay();
}

void setAlarm()
{
  delay(100);
  int currentClockSelected = 0; // 0 is 10 hour, 1 is 1 hour, 2 is 10 min etc.
  //bool buttonPressed = false; // used for exiting program
  while(true)
  {
    buttonState = digitalRead(buttonPin);
    currentClockSelected = getPWMValue(currentClockSelected, 5);
    writeAlarmDisplay(currentClockSelected);
    resetDisplay();
    if (buttonState == HIGH)
    {
      unsigned long buttonPressedStart = millis();
      while (buttonState == HIGH)
      {
        if (millis() - buttonPressedStart >= 1000)
        {
          writeAlarmDisplay(-1);
          delay(200);
          alarmActive = true;
          return;
        }
        buttonState = digitalRead(buttonPin);
      }
      if (millis() - buttonPressedStart >= 500)
      {
        writeAlarmDisplay(-1);
        delay(200);
        return;
      }
      else
      {
        editAlarm(currentClockSelected);
      }
    }
  }
}

 
void setup() {
  Serial.begin(9600);

  pinMode(buttonPin, INPUT);
  
  display.begin(SSD1306_SWITCHCAPVCC, screenAddress);
  display.clearDisplay();
  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // used when you need to resync clock
  prepareDisplayChars();

}

void loop() {
  DateTime now = rtc.now();
  buttonState = digitalRead(buttonPin);
  Serial.print("button state: ");
  Serial.println(buttonState);
  
  Serial.print("current slide: ");
  Serial.println(currentSlide, DEC);
  currentSlide = getPWMValue(currentSlide, 1);
  
    if (alarmActive)
    alarmClockManager(now);

  if (alarmTriggered)
    alarmTriggeredDisp();

  if (sleepMode)
  {
    if (buttonState == LOW)
      currentSlide = -1;
    else
    {
      delay(200);
      sleepMode = false;
      currentSlide = 0;
    }
  }

  buttonState = digitalRead(buttonPin);
  
  Serial.print("sleep mode: ");
  Serial.println(sleepMode);
  if (currentSlide == 0)
  {
    resetDisplay();
    clockDisplay(now);
    if (buttonState == HIGH)
    {
      delay(200);
      resetDisplay();
      display.display();
      sleepMode = true;
    }
  }
  else if (currentSlide == 1)
  {
    resetDisplay();
    writeAlarmDisplay(-1);
    
    if (buttonState == HIGH)
    {
      alarmActive = false;
      resetDisplay();
      setAlarm();
      delay(100);
      prevSecond = now.second();
    }
    resetDisplay();
  }
  

}
