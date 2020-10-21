#include <esp_deep_sleep.h>
#include <stdio.h>
#include <esp_spi_flash.h>
#include <driver/adc.h>
#include <BLEDevice.h>
#include "Arduino.h"

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */
#define DISCHARGE_LOOP_CNT 10000

static BLEUUID serviceUUID("5b187b6d-1dfa-4cb8-af03-f38e29b514c8");
static BLEUUID    charUUID("04b2bcfc-1b74-4203-8b10-6d8e09b5be2b");

static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static bool notifyFlag = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static bool isDischarge = false;
static bool isTransmit = false;
static bool isUpGrade = false;

uint16_t adcbuff[DISCHARGE_LOOP_CNT];
uint16_t cnt = 0;
long time_to_start = 0;
long adc_start_time = 0;
long adc_end_time = 0;
// uint16_t tbuff[SPI_FLASH_SEC_SIZE / 4];

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    notifyFlag = true;
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
  }
};

bool connectToServer() {
    BLEClient*  pClient  = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      pClient->disconnect();
      return false;
    }

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      pClient->disconnect();
      return false;
    }

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      String value = pRemoteCharacteristic->readValue();
      if(value[0] == '0'){
        isDischarge = true;
      }
      else if (value[0] == '1'){
        isUpGrade = true;
      }
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void discharge(){
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  adc1_config_width(ADC_WIDTH_MAX);
  delay(time_to_start);
  for (int i = 0; i < DISCHARGE_LOOP_CNT; i++)
  {
    // we shall do the calculation outside
    // adcbuff[i] = adc1_get_raw(ADC1_CHANNEL_0) / 4095 * 1.1;
    adcbuff[i] = adc1_get_raw(ADC1_CHANNEL_0);
  }
  isDischarge = false;
  isTransmit = true;
}

void transmitData(){
  if(notifyFlag){
    notifyFlag = false;
    String s = String(adcbuff[cnt]);
    cnt++;
    pRemoteCharacteristic->writeValue(s.c_str(),s.length());
    if(cnt == DISCHARGE_LOOP_CNT){
      isTransmit = false;
    }
  }
  else{
    pRemoteCharacteristic->writeValue("data ready");
  }
}

void upGrade(){
  isUpGrade = false;
}

void setup(){
  //Deep sleep mode settings, every sleep is TIME_TO_SLEEP seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  // rtc_gpio_isolate(GPIO_NUM_12); //wrover module only

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(5);
  pBLEScan->setWindow(10);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(1, false);
  if(doConnect){
    if(connectToServer()){
      pRemoteCharacteristic->writeValue("probe ready");
      delay(3);
      if(notifyFlag){
        time_to_start = atoi(pRemoteCharacteristic->readValue().c_str());
      }
    }
  }
}

void loop(){
  if(isDischarge){
    discharge();
  }
  if(isTransmit){
    transmitData();
  }
  if(isUpGrade){
    upGrade();
  }
  else{
    esp_deep_sleep_start(); 
  }
}
