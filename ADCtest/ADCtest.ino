//#include <driver/adc.h>
//
//void setup() {
//  
//  adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
//  adc1_config_width(ADC_WIDTH_MAX);
//  Serial.begin(115200);
//}
//
//void loop() {
//  // put your main code here, to run repeatedly:
//  int v = adc1_get_raw(ADC1_CHANNEL_0);
//  Serial.println(v);
//  delay(3000);
//}

// Get CPU cycle counter (240MHz)
uint32_t IRAM_ATTR cycles()
{
    uint32_t ccount;
    __asm__ __volatile__ ( "rsr     %0, ccount" : "=a" (ccount) );
    return ccount; 
}

void setup() {
  Serial.begin(115200);
}

#define SAMPLES 5000
//uint32_t t[SAMPLES];
uint32_t val[SAMPLES];

void loop() {
  int i;
  long t0 = millis();
  for(i=0; i<SAMPLES; i++)
  {
//    t[i] = cycles();
    val[i] = analogRead(25);    
  }
  Serial.println(millis()-t0);
  delay(500);
}
