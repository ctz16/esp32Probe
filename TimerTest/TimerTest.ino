#include <esp_timer.h>
#include <esp_deep_sleep.h>
#include <driver/adc.h>

uint64_t startTime = 0;
uint64_t endTime = 0;
uint16_t v = 0;

void setup() {
  Serial.begin(115200);
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  adc1_config_width(ADC_WIDTH_MAX);
  esp_deep_sleep_start();
}

void loop() {
  // put your main code here, to run repeatedly:
  startTime = esp_timer_get_time();
  for(int i = 0; i<10000; i++){
    v = adc1_get_raw(ADC1_CHANNEL_0);
  }
  endTime = esp_timer_get_time();
  long delTime = endTime-startTime;
  Serial.println(delTime);
  delay(1000);
}
