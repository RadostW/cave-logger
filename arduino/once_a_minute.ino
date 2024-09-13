// PINOUT
//
// Arduino Nano:
// ----------------------
// DS3231:  32K pin            -> .
//          SQW pin (green)    -> D2 pin
//          SDA pin (yellow)   -> A4 pin
//          SCL pin (blue)     -> A5 pin
//          VCC pin (red)      -> 3V3 pin
//          GND pin (black)    -> GND pin
//
//
// QWIIC:   SDA cable (blue)   -> A4 pin
//          SCL cable (yellow) -> A5 pin
//          3V3 cable (red)    -> 3V3 pin
//          GND cable (black)  -> GND pin
//
// SDcard:  GND pin (black)    -> GND pin
//          VCC pin (red)      -> 3V3 pin
//          MISO pin (purple)  -> D12 pin
//          MOSI pin (blue)    -> D11 pin
//          SCK pin  (green)   -> D13 pin
//          CS  pin  (orange)  -> D10 pin

// CONSTANTS
//
//define the number of hours mins and seconds between readings.
int hrs = 0;
int mins = 0;
int secs = 1;

#define CLOCK_INTERRUPT_PIN 2
const int chipSelect = 4;

// CODE
//
#include <RTClib.h>
#include <LowPower.h>
#include <SparkFun_MicroPressure.h>
#include <SPI.h>
#include <SD.h>

// GLOBALS
RTC_DS3231 rtc;
SparkFun_MicroPressure mpr;
File outputFile;

void setupRTC()
{
  // Real time clock
  while(!rtc.begin())
  {
    Serial.println(F("Cannot connect to RTC."));
  }
  Serial.println(F("[dbg] RTC connected."));
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //to set clock time to time of compile
  
  //we don't need the 32K Pin, so disable it
  rtc.disable32K();

  // Making it so, that the alarm will trigger an interrupt
  pinMode(CLOCK_INTERRUPT_PIN, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), onAlarm, FALLING);

  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset on reboot/recompile
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);

  // stop oscillating signals at SQW Pin
  // otherwise setAlarm1 will fail
  rtc.writeSqwPinMode(DS3231_OFF);

  // turn off alarm 2 (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go overlooked
  rtc.disableAlarm(2);
}

void setupPressure()
{
  while(!mpr.begin())
  {
    Serial.println(F("Cannot connect to pressure sensor."));    
  }
  Serial.println(F("[dbg] Pressure sensor connected."));
}

void setupCard()
{
  while (!SD.begin(chipSelect)) {
    Serial.println(F("[dbg] Cannot connect to SD card."));    
  }

  Serial.println(F("[dbg] SD card connected."));

  outputFile = SD.open("RESETLOG.txt", FILE_WRITE);
  DateTime now = rtc.now();

  if (outputFile) {      
    outputFile.print(now.year(), DEC);
    outputFile.print('-');
    outputFile.print(now.month(), DEC);
    outputFile.print('-');
    outputFile.print(now.day(), DEC);
    outputFile.print('T');
    outputFile.print(now.hour(), DEC);
    outputFile.print(':');
    outputFile.print(now.minute(), DEC);
    outputFile.print(':');
    outputFile.print(now.second(), DEC);
    outputFile.print(" ");  
    outputFile.println("Device restart.");
    // close the file:
    outputFile.close();    
  } else {    
    Serial.println(F("error opening RESETLOG.TXT"));
  }
  
}

void setup()
{
  // Setup Serial connection
  Serial.begin(9600);  
  
  Serial.println(F("================="));
  Serial.println(F("Warsaw Speleoclub"));
  Serial.println(F("   depth logger  "));
  Serial.println(F("    RW and SM    "));
  Serial.println(F("================="));

  setupRTC();
  setupPressure();
  setupCard();

  testRTCInterrupt();
  
  Serial.println(F("[dbg] Setup complete."));
  Serial.println("");
  
}

void sleepUntilInterrupt() {
  // Allow wake up pin to trigger interrupt on low.
  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), wakeUp, LOW);

  // Enter power down state with ADC and BOD module disabled.
  // Wake up when wake up pin is low.
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  // Disable external pin interrupt on wake up pin.
  detachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN));
}


void wakeUp()
{  
  // Blank handler (can fire multiple times per alarm)
}

void printTimeFromClock() {
  DateTime now = rtc.now();
  
  Serial.print(now.year(), DEC);
  Serial.print('-');
  Serial.print(now.month(), DEC);
  Serial.print('-');
  Serial.print(now.day(), DEC);
  Serial.print('T');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  delay(100);
}

void printTemperatureFromClock() {
  Serial.print("Temperature: ");
  Serial.print(rtc.getTemperature());
  Serial.println(" C");
  delay(100);
}

void printPressureFromSensor() {
  double pressurePascal = mpr.readPressure(PA);
  double standardPressure = 103000; // Pascal
  double hydrostaticGradient = 9800; // Pascal / meter
  Serial.print(pressurePascal,1);
  Serial.print(" Pa ");
  Serial.print(100*(pressurePascal - standardPressure) / hydrostaticGradient,2);
  Serial.print(" cm of water");
  Serial.println("");
  delay(100);
}

void printDepthShort() {
  double pressurePascal = mpr.readPressure(PA);
  double standardPressure = 103000; // Pascal
  double hydrostaticGradient = 9800; // Pascal / meter
  Serial.println(100*(pressurePascal - standardPressure) / hydrostaticGradient,2);
  delay(100);
}

void testRTCInterrupt()
{
  Serial.println(F("[dbg] Testing RTC interrupt."));
  delay(100);
  
  if (rtc.alarmFired(1)) 
  {
    rtc.clearAlarm(1);    
  }
  
  bool alarmSet = rtc.setAlarm1(
        rtc.now() + TimeSpan(0, 0, 0, 1), //1 second
        DS3231_A1_Minute 
      );
      
  if (!alarmSet) 
  {
    Serial.println(F("Error, alarm wasn't set!"));
  }

  attachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN), wakeUp, LOW);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);  
  detachInterrupt(digitalPinToInterrupt(CLOCK_INTERRUPT_PIN));

  Serial.println(F("[dbg] RTC interrupt success."));
}

void alarmReset()
{
  // check to see if the alarm flag is set (also resets the flag if set)
  if (rtc.alarmFired(1)) 
  {
    rtc.clearAlarm(1);    
  }
  
  bool alarmSet = rtc.setAlarm1(
        rtc.now() + TimeSpan(0, hrs, mins, secs), //sets the delay time in the following format: day, hour, min, second
        DS3231_A1_Minute // this mode triggers the alarm when the seconds match. See Doxygen for other options
      );
      
  if (!alarmSet) 
  {
    Serial.println(F("Error, alarm wasn't set!"));
  }   
}

void writeToSD()
{
  double pressurePascal = mpr.readPressure(PA);
  double standardPressure = 103000; // Pascal
  double hydrostaticGradient = 9800; // Pascal / meter
  
  outputFile = SD.open("DATALOG.txt", FILE_WRITE);
  DateTime now = rtc.now();

  if (outputFile) {      
    outputFile.print(now.year(), DEC);
    outputFile.print('-');
    outputFile.print(now.month(), DEC);
    outputFile.print('-');
    outputFile.print(now.day(), DEC);
    outputFile.print('T');
    outputFile.print(now.hour(), DEC);
    outputFile.print(':');
    outputFile.print(now.minute(), DEC);
    outputFile.print(':');
    outputFile.print(now.second(), DEC);
    outputFile.print(" ");  
    
    outputFile.print(pressurePascal,1);
    outputFile.print(" ");
    outputFile.print(100*(pressurePascal - standardPressure) / hydrostaticGradient,2);    
    outputFile.print(" ");
    
    outputFile.print(rtc.getTemperature());
    outputFile.println("");
    
    outputFile.close();
    delay(100);    
  } else {    
    Serial.println(F("error opening DATA_LOG.TXT"));
    delay(100);
  }
}

void loop()
{  
  alarmReset();
  printTimeFromClock();
  printPressureFromSensor();
  printTemperatureFromClock();
  writeToSD();
  //printDepthShort();
  sleepUntilInterrupt();
}
