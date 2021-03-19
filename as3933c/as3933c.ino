#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <esp_deep_sleep.h>
#include <driver/adc.h>
#include <driver/i2s.h>
#include <BLEDevice.h>
#include "SPI.h"
#include "as3933.h"

// #define SERIAL_DEBUG
#define BLE_DEBUG

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 60          /* Time ESP32 will go to sleep (in seconds) */
#define WORKING_TIME 600000       /* OTA update working time (ms)*/

const char *host = "esp32";
const char *ssid = "SUNIST-6";
const char *password = "sunist1234";

WebServer server(80);

SPIClass *hspi = new SPIClass(HSPI);
As3933 asTag(hspi, 15);

static BLEUUID serviceUUID("5b187b6d-1dfa-4cb8-af03-f38e29b514c8");
static BLEUUID charUUID("04b2bcfc-1b74-4203-8b10-6d8e09b5be2b");

static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static bool notifyFlag = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
static BLEClient *pClient;
long sys_start_time = 0;

/********** Web Server **********/

/*
 * Server Index Page
 */

const char *serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<div id='dif'>Hello Earth</div>"
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

void ServerInit()
{
  // Connect to WiFi network
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }
#ifdef BLE_DEBUG
  if (notifyFlag)
  {
    notifyFlag = false;
    if (connected)
    {
      pRemoteCharacteristic->writeValue(WiFi.localIP().toString().c_str());
    }
  }
#endif
#ifdef SERIAL_DEBUG
  Serial.println(WiFi.localIP().toString().c_str());
#endif

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host))
  { //http://esp32.local
    while (1)
    {
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
  server.on(
      "/update", HTTP_POST, []() {
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
    ESP.restart(); }, []() {
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
    } });
  server.begin();
#ifdef SERIAL_DEBUG
  Serial.println("Server begin");
#endif
}

/********** BLE **********/

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
    {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  }   // onResult
};    // MyAdvertisedDeviceCallbacks

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
  }

  void onDisconnect(BLEClient *pclient)
  {
    connected = false;
#ifdef SERIAL_DEBUG
    Serial.println("onDisconnect");
#endif
  }
};

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  notifyFlag = true;
}

bool connectToServer()
{
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
#ifdef SERIAL_DEBUG
    Serial.println("Failed to find our service UUID");
#endif
    pClient->disconnect();
    return false;
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr)
  {
#ifdef SERIAL_DEBUG
    Serial.println("Failed to find our characteristic UUID");
#endif
    pClient->disconnect();
    return false;
  }

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead())
  {
    String value = pRemoteCharacteristic->readValue().c_str();
  }

  if (pRemoteCharacteristic->canNotify())
  {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
#ifdef SERIAL_DEBUG
    Serial.println("notify registrited!");
#endif
  }

  connected = true;
  return true;
}

void BLEinit()
{
  BLEDevice::init("probeclient");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 2 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(50);
  pBLEScan->setWindow(200);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(2, false);
  if (doConnect)
  {
    if (connectToServer())
    {
#ifdef SERIAL_DEBUG
      Serial.println("probe ready");
#endif
      pRemoteCharacteristic->writeValue("probe ready");
    }
    else
    {
#ifdef SERIAL_DEBUG
      Serial.println("connection failed");
#endif
    }
  }
#ifdef SERIAL_DEBUG
  else
  {
    Serial.println("BLE server not found");
  }
#endif
  doConnect = false;
}

void setup(void)
{
  //   esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  //   esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 1);
#ifdef SERIAL_DEBUG
  Serial.begin(115200);
  Serial.println("BLE begin");
#endif
  BLEinit();
#ifdef BLE_DEBUG
  if (connected)
  {
    pRemoteCharacteristic->writeValue("test1: Checking AS3933 functionality");
  }
#endif

  if (!asTag.begin(125000))
  {
#ifdef BLE_DEBUG
    if (connected)
    {
      pRemoteCharacteristic->writeValue("Communication with AS3933 fails.");
    }
#endif
  }
  if (!asTag.setNrOfActiveAntennas(1) ||
      !asTag.setListeningMode(As3933::LM_STANDARD) ||
      !asTag.setFrequencyDetectionTolerance(As3933::FDT_BIG) ||
      !asTag.setAgc(As3933::AGC_UP_DOWN, As3933::GR_NONE) ||
      !asTag.setAntennaDamper(As3933::DR_NONE) ||
      !asTag.setWakeUpProtocol(As3933::WK_FREQ_DET_ONLY))
  {
#ifdef BLE_DEBUG
    if (connected)
    {
      pRemoteCharacteristic->writeValue("setup failed");
    }
#endif
  }
  else
  {
#ifdef BLE_DEBUG
    if (connected)
    {
      pRemoteCharacteristic->writeValue("wireless wakeup set");
    }
#endif
  }
  ServerInit();
  sys_start_time = millis();
}

void loop(void)
{
  while (millis() - sys_start_time < WORKING_TIME)
  {
    server.handleClient();
    delay(1);
  }
  asTag.clear_wake();
  esp_deep_sleep_start();
}
