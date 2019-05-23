// For Data Capturing
#include <SPI.h>
#include <SD.h>

#define SdMosiPin 11
#define SdMisoPin 12
#define SdClkPin 13
#define SdCsPin 4

File dataFile;

// For Real Time Capture
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

#define RtcSdaPin A4
#define RtcSclPin A5

// For Temperature & Humidity
#include <DHT.h>
#include <DHT_U.h>

#define DhtPin 8
#define DhtType DHT11

DHT dht(DhtPin, DhtType);

// For Opertating the Seed Starter Garden
#define soilPinA A0
#define waterPinA A1
#define pumpPinA 10

#define soilPinB A2
#define waterPinB A3
#define pumpPinB 9

// Global Properties
unsigned long lastWaterA = 0;
unsigned long lastWaterB = 0;
unsigned long lastWrite = 0;
bool wateringA = false;
bool wateringB = false;
long waterFrequency = 60000; // 60,000 milliseconds is 1 Minute
long writeFrequency = 300000; // 300,000 milliseconds is 5 Minutes

void setup() {
  Serial.begin(9600);

  // TUBE A
  pinMode(soilPinA, INPUT);
  pinMode(waterPinA, INPUT);
  pinMode(pumpPinA, OUTPUT);

  // TUBE B
  pinMode(soilPinB, INPUT);
  pinMode(waterPinB, INPUT);
  pinMode(pumpPinB, OUTPUT);

  // Arduino led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Init libraries
  SD.begin(4);
  rtc.begin();
  dht.begin();

  // Kick off data capture
  CaptureData();
}

void loop() {
  // Properties
  unsigned long currentTime = millis();
  
  int waterLevelA = analogRead(waterPinA);
  int soilMoistureA = analogRead(soilPinA);

  int waterLevelB = analogRead(waterPinB);
  int soilMoistureB = analogRead(soilPinB);

  // Start watering tube A
  if(plantsNeedWater(soilMoistureA) && !wateringA && !waterLevelFull(waterLevelA) && iCanWaterAgain(currentTime, lastWaterA)){
    digitalWrite(pumpPinA, HIGH);
    wateringA = true;
    CaptureData();
  }

  // Start watering tube B
  if(plantsNeedWater(soilMoistureB) && !wateringB && !waterLevelFull(waterLevelB) && iCanWaterAgain(currentTime, lastWaterB)){
    digitalWrite(pumpPinB, HIGH);
    wateringB = true;
    CaptureData();
  }

  // Stop watering tube A
  if(waterLevelFull(waterLevelA) && wateringA){
    lastWaterA = currentTime;
    digitalWrite(pumpPinA, LOW);
    wateringA = false;
    CaptureData();
  }

  // Stop watering tube B
  if(waterLevelFull(waterLevelB) && wateringB){
    lastWaterB = currentTime;
    digitalWrite(pumpPinB, LOW);
    wateringB = false;
    CaptureData();
  }

  // Capture plant data every 5 minutes 
  if(currentTime - lastWrite > writeFrequency){
    lastWrite = currentTime;
    CaptureData();
  }
}

bool plantsNeedWater(int soilMoisture){
  return soilMoisture > 490;
}

bool waterLevelFull(int waterLevel){
  return waterLevel > 100;
}

bool iCanWaterAgain(int currentTime, int lastWater){
  return currentTime - lastWater > waterFrequency;
}

String GetDate() {
  DateTime now = rtc.now();
  String comma = String(',');
  String colon = String(':');
  String dash = String('-');
  String year = String(now.year());
  String month = String(now.month());
  String day = String(now.day());
  String hour = String(now.hour());
  String minute = String(now.minute());
  String second = String(now.second());
  
  return String(year + dash + month + dash + day + comma + hour + colon + minute + colon + second);
}

int GetHumid() {
  int humidity = dht.readHumidity();

  if(isnan(humidity)){
    return 0;
  }
  return humidity;
}

float GetTemp() {
  float temperature = dht.readTemperature();
  
  if(isnan(temperature)){
    return 0;
  }

  return temperature;
}

void CaptureData() {
  int waterLevelA = analogRead(waterPinA);
  int soilMoistureA = analogRead(soilPinA);
  int waterLevelB = analogRead(waterPinB);
  int soilMoistureB = analogRead(soilPinB);
  
  dataFile = SD.open("seedData.csv", FILE_WRITE);
  if(dataFile){
    dataFile.print(GetDate());
    dataFile.print(",");
    dataFile.print(GetTemp());
    dataFile.print(",");
    dataFile.print(GetHumid());
    dataFile.print(",");
    dataFile.print(soilMoistureA);
    dataFile.print(",");
    dataFile.print(waterLevelA);
    dataFile.print(",");
    dataFile.println(wateringA);
    dataFile.print(",");
    dataFile.print(soilMoistureB);
    dataFile.print(",");
    dataFile.print(waterLevelB);
    dataFile.print(",");
    dataFile.println(wateringB);
    dataFile.close();

    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}
