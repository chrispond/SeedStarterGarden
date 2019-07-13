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
#define DhtType DHT11

DHT dht(DhtPin, DhtType);

// For Opertating the Seed Starter Garden
#define soilPinA A0
#define waterPinA A1
const int pumpPinA = 10;

#define soilPinB A2
#define waterPinB A3
const int pumpPinB = 9;

// Global Properties
bool firstTimeLoaded = false;
bool capturingData = false;
bool wateringA = false;
bool wateringB = false;
long waterFrequency = 60000; // 60,000 milliseconds is 1 Minute
long writeFrequency = 300000; // 300,000 milliseconds is 5 Minutes

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

  // Arduino led
  pinMode(LED_BUILTIN, OUTPUT);

  // Init libraries
  SD.begin(SdCsPin);
  rtc.begin();
  dht.begin();

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

  Serial.println(waterLevelA);

  if(!firstTimeLoaded){
    digitalWrite(pumpPinA, HIGH);
    digitalWrite(pumpPinB, HIGH);
    firstTimeLoaded = true;

    // Delay so that the sensors have time to level out
    delay(writeFrequency);
  }

  // Start watering tube A
  if(plantsNeedWater(soilMoistureA) && !wateringA && !waterLevelFull(waterLevelA) && iCanWaterAgain(currentTime, lastWaterA)){
    digitalWrite(pumpPinA, LOW);
    wateringA = true;
    CaptureData(dataFileNameA, soilMoistureA, waterLevelA, wateringA);
  }

  // Start watering tube B
  if(plantsNeedWater(soilMoistureB) && !wateringB && !waterLevelFull(waterLevelB) && iCanWaterAgain(currentTime, lastWaterB)){
    digitalWrite(pumpPinB, LOW);
    wateringB = true;
    CaptureData(dataFileNameB, soilMoistureB, waterLevelB, wateringB);
  }

  // Stop watering tube A
  if(waterLevelFull(waterLevelA) && wateringA){
    lastWaterA = currentTime;
    digitalWrite(pumpPinA, HIGH);
    wateringA = false;
    CaptureData(dataFileNameA, soilMoistureA, waterLevelA, wateringA);
  }

//  // Stop watering tube B
  if(waterLevelFull(waterLevelB) && wateringB){
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

bool plantsNeedWater(int soilMoisture){
  return soilMoisture >= 490;
}

bool waterLevelFull(int waterLevel){
  return waterLevel >= 100;
}

bool iCanWaterAgain(int currentTime, int lastWater){
  return currentTime - lastWater >= waterFrequency;
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

void CaptureData(String dataFileName, int soilMoisture, int waterLevel, bool watering) {
  
  dataFile = SD.open(dataFileName, FILE_WRITE);
  
  if(dataFile && !capturingData){
    const int constrainedSoilMoisture = constrain(soilMoisture, 350, 520);
    const int parsedSoilMoisture = map(constrainedSoilMoisture, 350, 520, 100, 0);

    capturingData = true;
    dataFile.print(GetDate());
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

    digitalWrite(LED_BUILTIN, HIGH);
  }else if(capturingData){
    CaptureData(dataFileName, soilMoisture, waterLevel, watering);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
