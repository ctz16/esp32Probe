// #define SERIAL_DEBUG
// #define BLE_DEBUG

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Arduino.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "5b187b6d-1dfa-4cb8-af03-f38e29b514c8"
#define CHARACTERISTIC_UUID "04b2bcfc-1b74-4203-8b10-6d8e09b5be2b"

BLEDevice *myBLE;
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
char buffer[30];
String cmd;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      Serial.println(pCharacteristic->getValue().c_str());
      pCharacteristic->notify();
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
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    // read the incoming byte:
    cmd = Serial.readString();
    if(cmd[0] == 'd' || cmd[0] =='u' || cmd[0]=='t'){
      BLEinit(cmd);
    }
  }
}
