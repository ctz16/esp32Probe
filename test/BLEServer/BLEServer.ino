/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/
#define _GLIBCXX_USE_CXX11_ABI 0
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "5b187b6d-1dfa-4cb8-af03-f38e29b514c8"
#define CHARACTERISTIC_UUID "04b2bcfc-1b74-4203-8b10-6d8e09b5be2b"

BLEDevice *myBLE;
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
char buffer[30];

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
//      std::string value = pCharacteristic->getValue();

//      if (value.length() > 0) {
//        Serial.println("*********");
//        Serial.print("New value: ");
//        for (int i = 0; i < value.length(); i++)
//          Serial.print(value[i]);
//
//        Serial.println();
//        Serial.println("*********");
//      }
    // int value = atoi(pCharacteristic->getValue().c_str());
    // Serial.print("new value: ");
    // Serial.println(value);
    Serial.println(pCharacteristic->getValue().c_str());
//    pCharacteristic->setValue("OK");
    pCharacteristic->notify();
    }

};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

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
  pCharacteristic->setValue("Hello World says Neil, please say hello to me");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!!");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(3000);
  Serial.print("value is: ");
  Serial.println(pCharacteristic->getValue().c_str());
}
