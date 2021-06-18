#include <Arduino.h>
#include <esp_deep_sleep.h>
#include <driver/rtc_io.h>
#include "SPI.h"
#include "bma400.h"

#define INT1PIN 33
#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

SPIClass* hspi = new SPIClass(HSPI);
BMA400 bma400(hspi);

void setup()
{
    hspi->begin();
    pinMode(HSPICS0, OUTPUT);
//    pinMode(INT1PIN, INPUT);
    Serial.begin(115200);
    
    digitalWrite(HSPICS0,LOW);
    delay(10);
    digitalWrite(HSPICS0,HIGH);
    delay(10);
    
    while (1)
    {
        if (bma400.isConnection())
        {
            bma400.initialize();
            Serial.println("BMA400 is connected");
            break;
        }
        else
        {
            Serial.println("BMA400 is not connected");
            delay(3000);
        }
    }
    bma400.enable_shake_interrupt();
    
    delay(2000);
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
    //Print the wakeup reason for ESP32
    print_wakeup_reason();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    rtc_gpio_isolate(GPIO_NUM_12);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33,0);
    Serial.println("going to sleep");
    esp_deep_sleep_start();
}

void loop()
{
}
