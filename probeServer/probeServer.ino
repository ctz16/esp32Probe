#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <esp_deep_sleep.h>
#include <Arduino.h>

// #define SERIAL_DEBUG
// #define BLE_DEBUG

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "5b187b6d-1dfa-4cb8-af03-f38e29b514c8"
#define CHARACTERISTIC_UUID "04b2bcfc-1b74-4203-8b10-6d8e09b5be2b"

#define TIME_TO_TRANSMIT 40000

BLEDevice *myBLE;
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
char buffer[30];
String cmd;
String time_to_start;
String s;
bool transmitFlag = false;
int cnt = 0;
int databuff[10000];
long start_time = 0;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      s = pCharacteristic->getValue().c_str();
      #ifdef SERIAL_DEBUG
        Serial.print("notify message:");
        Serial.println(s);
      #endif
      if (transmitFlag){
        databuff[cnt] = s.toInt();
        cnt++;
      }
      else{
        if (s[0]=='o'){
          pCharacteristic->setValue(time_to_start.c_str());
        }
        else if(s[0]=='x'){
          transmitFlag = true;
          cnt=0;
        }
        else{
          Serial.println(s);
        }
      }
      pCharacteristic->notify();
      #ifdef SERIAL_DEBUG
        Serial.println("notified");
      #endif
    }
};

void BLEinit(String cmd){
  myBLE->init("ctzESP32");
  pServer = myBLE->createServer();
  pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                      CHARACTERISTIC_UUID,
                                      BLECharacteristic::PROPERTY_READ   |
                                      BLECharacteristic::PROPERTY_WRITE  |
                                      BLECharacteristic::PROPERTY_NOTIFY |
                                      BLECharacteristic::PROPERTY_INDICATE
                                       );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue(cmd.c_str());
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting work!");
  start_time = millis();
}

void loop() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    cmd = Serial.readString();
    switch (cmd[0])
    {
    case 'd':
      Serial.println("discharge mode");
      time_to_start = Serial.readString();
      BLEinit(cmd);
      Serial.print("BLE start, time to start:");
      Serial.println(time_to_start.c_str());
      break;
    case 'o':
      Serial.println("sleep mode");
      BLEinit(cmd);
      delay(60000);
      break;
    default:
      Serial.println("deep sleep start");
      esp_deep_sleep_start();
      break;
    }
  }
  else if (millis()-start_time > TIME_TO_TRANSMIT){
    Serial.println("x");
    for (int i = 0; i < cnt; i++)
    {
      Serial.println(databuff[i]);
    }
    Serial.print("cnt: ");
    Serial.println(cnt);
    Serial.println("deep sleep start");
    esp_deep_sleep_start();
  }
}
