// For Data Capturing
#include <SPI.h>
#include <SD.h>

File dataFile;

// MOSI - pin 11
// MISO - pin 12
// CLK - pin 13
const int SdCsPin = 4;
String dataFileNameA = "SEEDATAA.csv";
String dataFileNameB = "SEEDATAB.csv";

// For Real Time Capture
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

#define RtcSdaPin A4
#define RtcSclPin A5

// For Temperature & Humidity
#include <DHT.h>
#include <DHT_U.h>

const int DhtPin = 8;
#define DhtType DHT22

DHT dht(DhtPin, DhtType);

// For Opertating the Seed Starter Garden
#define soilPinA A0
#define waterPinA A1
#define pumpPinA 10
//
#define soilPinB A2
#define waterPinB A3
#define pumpPinB 9

// Global Properties
bool firstTimeLoaded = false;
bool capturingData = false;
bool wateringA = false;
bool wateringB = false;
long writeFrequency = 900000; // 300,000 milliseconds is 15 Minutes

unsigned long serialWrite = 0;
unsigned long lastWaterA = 0;
unsigned long lastWaterB = 0;
unsigned long lastWriteA = 0;
unsigned long lastWriteB = 0;

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

  // Init libraries
  SD.begin(SdCsPin);
  rtc.begin();
  dht.begin();

  // Set the proper time if it isn't set
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  CaptureData(dataFileNameA, 0, 0, false);
  delay(1000);
  CaptureData(dataFileNameB, 0, 0, false);
}

void loop() {
  // Properties
  unsigned long currentTime = millis();
  
  int waterLevelA = analogRead(waterPinA);
  int soilMoistureA = analogRead(soilPinA);

  int waterLevelB = analogRead(waterPinB);
  int soilMoistureB = analogRead(soilPinB);

  if(!firstTimeLoaded){
    digitalWrite(pumpPinA, HIGH);
    digitalWrite(pumpPinB, HIGH);
    firstTimeLoaded = true;

    // Delay so that the sensors have time to level out
    delay(30000);
  }

  // Start watering tube A
  if(plantsNeedWaterA(soilMoistureA) && !wateringA && !waterLevelFull(waterLevelA)){
    digitalWrite(pumpPinA, LOW);
    lastWaterA = currentTime;
    wateringA = true;
    CaptureData(dataFileNameA, soilMoistureA, waterLevelA, wateringA);
  }

  // Start watering tube B
  if(plantsNeedWaterB(soilMoistureB) && !wateringB && !waterLevelFull(waterLevelB)){
    digitalWrite(pumpPinB, LOW);
    lastWaterB = currentTime;
    wateringB = true;
    CaptureData(dataFileNameB, soilMoistureB, waterLevelB, wateringB);
  }

  // Stop watering tube A
  if(waterLevelFull(waterLevelA) && wateringA || overRideStopWatering(currentTime, lastWaterA)){
    digitalWrite(pumpPinA, HIGH);
    wateringA = false;
    CaptureData(dataFileNameA, soilMoistureA, waterLevelA, wateringA);
  }

  // Stop watering tube B
  if(waterLevelFull(waterLevelB) && wateringB || overRideStopWatering(currentTime, lastWaterB)){
    lastWaterB = currentTime;
    digitalWrite(pumpPinB, HIGH);
    wateringB = false;
    CaptureData(dataFileNameB, soilMoistureB, waterLevelB, wateringB);
  }

  // Capture plant data every 5 minutes 
  if(currentTime - lastWriteA > writeFrequency && !capturingData){
    lastWriteA = currentTime;
    CaptureData(dataFileNameA, soilMoistureA, waterLevelA, wateringA);
  }else if(currentTime - lastWriteB > writeFrequency && !capturingData){
    lastWriteB = currentTime;
    CaptureData(dataFileNameB, soilMoistureB, waterLevelB, wateringB);
  }
  
  delay(500);
}

bool plantsNeedWaterA(int soilMoisture){  
  return soilMoisture >= 495;
}

bool plantsNeedWaterB(int soilMoisture){  
  return soilMoisture >= 450;
}

bool waterLevelFull(int waterLevel){
  return waterLevel >= 10;
}

bool overRideStopWatering(unsigned long currentTime, unsigned long waterStartTime){
  return (currentTime - waterStartTime > writeFrequency);
}

String GetDate() {
  DateTime now = rtc.now();
  int year = int(now.year());
  int month = int(now.month());
  int day = int(now.day());

  char dateStamp[100];
  sprintf(dateStamp, "%d-%02d-%02d", year, month, day);
  
  return dateStamp;
}

String GetTime() {
  DateTime now = rtc.now();
  int hours = int(now.hour());
  int minutes = int(now.minute());
  int seconds = int(now.second());
  
  char timeStamp[100];
  sprintf(timeStamp, "%02d:%02d:%02d", hours, minutes, seconds);
  
  return timeStamp;
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

void CaptureData(String dataFileName, int soilMoisture, int waterLevel, bool watering) {
  
  dataFile = SD.open(dataFileName, FILE_WRITE);
  
  if(dataFile && !capturingData){
    const int constrainedSoilMoisture = constrain(soilMoisture, 350, 520);
    const int parsedSoilMoisture = map(constrainedSoilMoisture, 350, 520, 100, 0);

    capturingData = true;
    dataFile.print(GetDate());
    dataFile.print(",");
    dataFile.print(GetTime());
    dataFile.print(",");
    dataFile.print(GetTemp());
    dataFile.print(",");
    dataFile.print(GetHumid());
    dataFile.print(",");
    dataFile.print(parsedSoilMoisture);
    dataFile.print(",");
    dataFile.print(waterLevel);
    dataFile.print(",");
    dataFile.println(watering);
    dataFile.close();
    capturingData = false;
  }else if(capturingData){
    CaptureData(dataFileName, soilMoisture, waterLevel, watering);
  }
}
