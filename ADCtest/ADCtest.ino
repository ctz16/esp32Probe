#include <driver/adc.h>

void setup() {
  
  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
  adc1_config_width(ADC_WIDTH_MAX);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  int v = adc1_get_raw(ADC1_CHANNEL_0);
  Serial.println(v);
  delay(3000);
}
