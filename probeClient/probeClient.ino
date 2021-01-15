#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_deep_sleep.h>
#include <esp_spi_flash.h>
#include <driver/adc.h>
#include <BLEDevice.h>

#define SERIAL_DEBUG 
#define BLE_DEBUG

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  300        /* Time ESP32 will go to sleep (in seconds) */
#define DISCHARGE_LOOP_CNT 500     /* total ADC samples */
#define WORKING_TIME 180000       /* updateworking time (ms)*/
#define TIME_TO_END 5000          /* time (in ms) between adc end to transmit start */

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

uint16_t adcbuff[DISCHARGE_LOOP_CNT];
uint16_t cnt = 0;
long time_to_start = 0;
long transmit_start_time = 0;
long adc_start_time = 0;
long start_time = 0;
char tbuff[10];
// uint16_t tbuff[SPI_FLASH_SEC_SIZE / 4];

/********** Web Server **********/

/*
 * Server Index Page
 */
 
const char* serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<div id='dif'>new text!!</div>"
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
  server.on("/serverIndex", HTTP_GET, []() {
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
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
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
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  adc1_config_width(ADC_WIDTH_BIT_12);
  if(notifyFlag){
    notifyFlag = false;
    #ifdef SERIAL_DEBUG
      Serial.println("notify recieved");
    #endif
    String value = pRemoteCharacteristic->readValue().c_str();
    time_to_start = value.toInt();
  }
  #ifdef SERIAL_DEBUG
    Serial.print("time to start: ");
    Serial.println(time_to_start);
  #endif
  delay(time_to_start);
  for (int i = 0; i < DISCHARGE_LOOP_CNT; i++)
  {
    // we shall do the calculation outside
    // adcbuff[i] = adc1_get_raw(ADC1_CHANNEL_0) / 4095 * 1.1;
    adcbuff[i] = adc1_get_raw(ADC1_CHANNEL_0);
  }
  delay(TIME_TO_END);
  isDischarge = false;
  if(connected){
    isTransmit = true;
  }
}

void transmitData(){
  if(connected){
    if(notifyFlag){
      notifyFlag = false;
      itoa(adcbuff[cnt],tbuff,10);
      pRemoteCharacteristic->writeValue(tbuff);
      cnt++;
      if(cnt == DISCHARGE_LOOP_CNT){
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
  while (millis()-start_time < WORKING_TIME){
    server.handleClient();
    delay(1);
  }
}

void setup(void) {
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  #ifdef SERIAL_DEBUG
    Serial.begin(115200);
    Serial.println("BLE begin");
  #endif
  BLEinit();
  start_time = millis();
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
    #ifdef SERIAL_DEBUG
      Serial.print("deep sleep start, wait for seconds: ");
      Serial.println(TIME_TO_SLEEP);
    #endif
    esp_deep_sleep_start();
  }
}
