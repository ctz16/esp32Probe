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
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

static bool isDischarge = false;
static bool isUpGrade = false;

double adcbuff[DISCHARGE_LOOP_CNT];
int tbuff[SPI_FLASH_SEC_SIZE / 4];
int adc_start_time = 0;
int adc_end_time = 0;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    //Serial.print("Notify callback for characteristic ");
    //Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    //Serial.print(" of data length ");
    //Serial.println(length);
    //Serial.print("data: ");
    //Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    //Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    //Serial.print("Forming a connection to ");
    //Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    //Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    //Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      //Serial.print("Failed to find our service UUID: ");
      //Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      //Serial.print("Failed to find our characteristic UUID: ");
      //Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      //Serial.print("The characteristic value was: ");
      //Serial.println(value.c_str());
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
    //Serial.print("BLE Advertised Device found: ");
    //Serial.println(advertisedDevice.toString().c_str());

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
  // spi_flash_init();
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  adc1_config_width(ADC_WIDTH_MAX);
  for (int i = 0; i < DISCHARGE_LOOP_CNT; i++)
  {
    // we shall do the calculation outside
    // adcbuff[i] = adc1_get_raw(ADC1_CHANNEL_0) / 4095 * 1.1;
    adcbuff[i] = adc1_get_raw(ADC1_CHANNEL_0);
  }
  delay(500);
  pRemoteCharacteristic->writeValue(adcbuff,sizeof(adcbuff));
}

void upGrade(){}

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
  pBLEScan->setInterval(10);
  pBLEScan->setWindow(50);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(1, false);
}

void loop(){
  if(isDischarge){
    // discharge main function
    discharge();
    isDischarge = false;
  }
  if(isUpGrade){
    upGrade();
  }
  else{
   esp_deep_sleep_start(); 
  }
}
