#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_deep_sleep.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>
#include <BLEDevice.h>
#include "SPI.h"
#include "bma400.h"

//#define SERIAL_DEBUG 
#define BLE_DEBUG

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
// #define TIME_TO_SLEEP  60        /* Time ESP32 will go to sleep (in seconds, timer wakeup) */
#define SAMPLE_NUM 20000     /* total ADC samples */
#define WORKING_TIME 180000       /* OTA update working time (ms)*/
#define INTERVAL_AFTER_DISCHARGE 1000          /* time (in ms) between adc end to transmit start */
#define PAC_LEN 100               /* data count in each package */
// #define I2S_SAMPLE_RATE 500000
#define ADC_INPUT ADC1_CHANNEL_0  /* pin SENSOR_VP */

RTC_DATA_ATTR int bootCount = 0;
SPIClass* hspi = new SPIClass(HSPI);
BMA400 bma400(hspi);

const char* host = "esp32";
const char* ssid = "SUNIST-6";
const char* password = "sunist1234";

WebServer server(80);

static BLEUUID serviceUUID("5b187b6d-1dfa-4cb8-af03-f38e29b514c8");
static BLEUUID    charUUID("04b2bcfc-1b74-4203-8b10-6d8e09b5be2b");

static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static bool notifyFlag = false;
static bool transmit_start_flag = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static BLEClient*  pClient;

static bool isDischarge = false;
static bool isTransmit = false;
static bool isUpGrade = false;

uint16_t adc_reading[SAMPLE_NUM];

uint16_t cnt = 0;
long interval_to_discharge = 0;
long transmit_start_time = 0;
long adc_start_time = 0;
long adc_end_time = 0;
long sys_start_time = 0;

/********** Web Server **********/

/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://gapis.geekzu.org/ajax/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<div id='dif'>Hello Mars!!!</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

void ServerInit(){
    // Connect to WiFi network
  WiFi.begin(ssid, password);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  if(notifyFlag){
    notifyFlag = false;
    #ifdef BLE_DEBUG
      if(connected){
        pRemoteCharacteristic->writeValue(WiFi.localIP().toString().c_str());
      }
    #endif
    #ifdef SERIAL_DEBUG
      Serial.println(WiFi.localIP().toString().c_str());
    #endif
  }
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    while (1) {
      delay(1000);
    }
  }
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    #ifdef BLE_DEBUG
      if(connected){
        pRemoteCharacteristic->writeValue("success, rebooting");
      }
    #endif
    #ifdef SERIAL_DEBUG
      Serial.println("success, rebooting");
    #endif
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
      } else {
      }
    }
  });
  server.begin();
  #ifdef SERIAL_DEBUG
    Serial.println("Server begin");
  #endif
}

/********** BLE **********/

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
    #ifdef SERIAL_DEBUG
      Serial.println("onDisconnect");
    #endif
  }
};

bool connectToServer() {
    pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      #ifdef SERIAL_DEBUG
        Serial.println("Failed to find our service UUID");
      #endif
      pClient->disconnect();
      return false;
    }

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      #ifdef SERIAL_DEBUG
        Serial.println("Failed to find our characteristic UUID");
      #endif
      pClient->disconnect();
      return false;
    }

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      String value = pRemoteCharacteristic->readValue().c_str();
      #ifdef SERIAL_DEBUG
        Serial.print("cmd recived:");
        Serial.println(value);
      #endif
      if(value[0] == 'd'){
        isDischarge = true;
      }
      else if (value[0] == 'u'){
        isUpGrade = true;
      }
    }

    if(pRemoteCharacteristic->canNotify()){
      pRemoteCharacteristic->registerForNotify(notifyCallback);
      #ifdef SERIAL_DEBUG
        Serial.println("notify registrited!");
      #endif
    }
      
    connected = true;
    return true;
}

void BLEinit(){
  BLEDevice::init("probeclient");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 2 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(50);
  pBLEScan->setWindow(200);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(2, false);
  if(doConnect){
    if(connectToServer()){
        #ifdef SERIAL_DEBUG
          Serial.println("probe ready");
        #endif
      pRemoteCharacteristic->writeValue("probe ready");
    }
    else{
        #ifdef SERIAL_DEBUG
          Serial.println("connection failed");
        #endif
    }
  }
  #ifdef SERIAL_DEBUG 
    else{
      Serial.println("BLE server not found");
    }
  #endif
  doConnect = false;
}

/********** Control **********/

void discharge(){
  pRemoteCharacteristic->writeValue("o");
  delay(50);
//  i2sInit();
	adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_11);
	adc1_config_width (ADC_WIDTH_12Bit);
  for (int i = 0; i < SAMPLE_NUM/10; i++){
    adc_reading[i] = analogRead(36);
  }
  unsigned long t = 0;
  if(notifyFlag){
    notifyFlag = false;
    #ifdef SERIAL_DEBUG
      Serial.println("notify recieved");
    #endif
    String value = pRemoteCharacteristic->readValue().c_str();
    interval_to_discharge = value.toInt();
    t = millis();
  }
  #ifdef SERIAL_DEBUG
    Serial.print("time to start: ");
    Serial.println(interval_to_discharge);
  #endif
  pClient->disconnect();
  BLEDevice::deinit();
  long time_to_start = interval_to_discharge - (millis()-t);
  if(time_to_start > 0){
    delay(time_to_start);
  }
  adc_reading[0] = adc1_get_raw(ADC1_CHANNEL_0);
//  portDISABLE_INTERRUPTS();
  adc_start_time = millis();
  for (int i = 0; i < SAMPLE_NUM; i++){
    adc_reading[i] = analogRead(36);  //sensor vp
//    adc_reading[i] = adc1_get_raw(ADC1_CHANNEL_0);
  }
  // we shall do the calculation outside:
  // adc_results[i] = 1.1* ( (float) (adc_reading[i]& 0x0FFF)) /0x0FFF;
  adc_end_time = millis();
//  portENABLE_INTERRUPTS();
  delay(INTERVAL_AFTER_DISCHARGE);
  isDischarge = false;
  BLEinit();
  if(connected){
    isTransmit = true;
  }
  #ifdef SERIAL_DEBUG
  else{
    Serial.println("BLE reconnect failed");
  }
  #endif
}

void transmitData(){
  if(connected){
    if(notifyFlag){
      notifyFlag = false;
      String s = "";
      for (int i = 0; i < PAC_LEN; i++)
      {
        s += (adc_reading[PAC_LEN*cnt+i] & 0x0FFF);
        s += ",";
      }
      cnt++;
      pRemoteCharacteristic->writeValue(s.c_str());
      if(cnt == SAMPLE_NUM / PAC_LEN){
        delay(50);
        char tbuff[10];
        itoa(adc_end_time-adc_start_time,tbuff,10);
        pRemoteCharacteristic->writeValue(tbuff);
        #ifdef SERIAL_DEBUG
          Serial.print("adc sample cost: ");
          Serial.println(tbuff);
        #endif
        delay(50);
        pRemoteCharacteristic->writeValue("Transmition End");
        isTransmit = false;
        transmit_start_flag = false;
        #ifdef SERIAL_DEBUG
          Serial.print("transmition finish, cost:");
          Serial.println(millis()-transmit_start_time);
        #endif        
        cnt=0;
      }
    }
    else if(!transmit_start_flag){
      transmit_start_time = millis();
      pRemoteCharacteristic->writeValue("x");
      transmit_start_flag = true;
      #ifdef SERIAL_DEBUG
        Serial.println("transmit start");
      #endif
      delay(50);
    }
  }
  else{
    isTransmit = false;
  }
}

void upGrade(){
  isUpGrade = false;
  ServerInit();
  while (millis()-sys_start_time < WORKING_TIME){
    server.handleClient();
    delay(1);
  }
}

void setup(void) {
  #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    Serial.println("BLE begin");
    Serial.println("Boot number: " + String(bootCount));
  #endif
  ++bootCount;
  pinMode(HSPICS0, OUTPUT);
  digitalWrite(HSPICS0,LOW);
  delay(10);
  digitalWrite(HSPICS0,HIGH);
  delay(10);
  BLEinit();
  sys_start_time = millis();
}

void loop(void) {
  if(isDischarge){
    #ifdef SERIAL_DEBUG
      Serial.println("discharge");
    #endif
    discharge();
  }
  else if(isTransmit){
    transmitData();
  }
  else if(isUpGrade){
    #ifdef SERIAL_DEBUG
      Serial.println("upgrade");
    #endif
    upGrade();
  }
  else{
    hspi->begin();
    if (bma400.isConnection())
    {
        bma400.initialize();
        bma400.enable_shake_interrupt();
        #ifdef SERIAL_DEBUG
          Serial.println("BMA400 is ready");
        #endif
    }
    #ifdef SERIAL_DEBUG
    else
    {
        Serial.println("BMA400 is not connected");
        delay(3000);
    }
    #endif
    #ifdef SERIAL_DEBUG
      Serial.print("deep sleep start, shake to wakeup ");
    #endif
//    esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_34,0);
    rtc_gpio_isolate(GPIO_NUM_12);
    esp_deep_sleep_start();
  }
}
