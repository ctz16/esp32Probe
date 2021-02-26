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

#define TIME_TO_TRANSMIT 60000
#define TIME_TO_UPDATE 180000

BLEDevice *myBLE;
BLEServer *pServer;
BLEService *pService;
BLECharacteristic *pCharacteristic;
char buffer[30];
String cmd;
String s[500];
bool transmitFlag = false;
int cnt = 0;
int time_to_start = 0;
long sys_start_time = 0;
bool readFlag = false;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      readFlag = true;
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
  sys_start_time = millis();
}

void loop() {
  /***** ble controller *****/
  if (readFlag){
    // data come in from probe client
    readFlag = false;
    s[cnt] = pCharacteristic->getValue().c_str();
    #ifdef SERIAL_DEBUG
      Serial.print("notify message:");
      Serial.println(s);
    #endif

    if (transmitFlag){
      // client data transmitting
      cnt++;
    }

    else{
      if (s[cnt][0]=='o'){
        // client asks for time to discharge
        char tbuff[10];
        itoa(time_to_start-(millis()-sys_start_time),tbuff,10);
        pCharacteristic->setValue(tbuff);
      }
      else if(s[cnt][0]=='x'){
        // client tells to transmit data
        transmitFlag = true;
        cnt=0;
      }
      else{
        // other messages
        Serial.println(s[cnt]);
      }
    }

    //ready for recieve next data
    pCharacteristic->notify();
    #ifdef SERIAL_DEBUG
      Serial.println("notified");
    #endif
  }

  /***** raspberry pi controller *****/
  else if (Serial.available() > 0) {
    // read the incoming byte:
    cmd = Serial.readString();
    switch (cmd[0])
    {
    case 'd':
      Serial.println("discharge mode");
      Serial.println("X"); //ask for time to discharge
      delay(50);
      s[0] = Serial.readString();
      sys_start_time = millis();
      time_to_start = s[0].toInt();
      BLEinit(cmd);
      Serial.print("BLE start, time to start:");
      Serial.println(time_to_start);
      break;
    case 'u':
      Serial.println("update mode");
      BLEinit(cmd);
      sys_start_time = millis();
      break;
    case 'o':
      Serial.println("sleep mode");
      BLEinit(cmd);
      delay(30000);
      esp_deep_sleep_start();
      break;
    default:
      Serial.println("deep sleep start");
      esp_deep_sleep_start();
      break;
    }
  }

  // transmit data to pi
  else if (millis()-sys_start_time > TIME_TO_TRANSMIT && cmd[0] == 'd'){
    Serial.println("x");
    for (int i = 0; i < cnt; i++)
    {
      Serial.println(s[i]);
    }
    Serial.print("cnt: ");
    Serial.println(cnt);
    Serial.println("deep sleep start");
    esp_deep_sleep_start();
  }

  else if (millis()-sys_start_time > TIME_TO_UPDATE && cmd[0] == 'u'){
    Serial.println("deep sleep start");
    esp_deep_sleep_start();
  }
}
