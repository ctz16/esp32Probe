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

//// Get CPU cycle counter (240MHz)
//uint32_t IRAM_ATTR cycles()
//{
//    uint32_t ccount;
//    __asm__ __volatile__ ( "rsr     %0, ccount" : "=a" (ccount) );
//    return ccount; 
//}
//
//void setup() {
//  Serial.begin(115200);
//}
//
//#define SAMPLES 5000
////uint32_t t[SAMPLES];
//uint32_t val[SAMPLES];
//
//void loop() {
//  int i;
//  long t0 = millis();
//  for(i=0; i<SAMPLES; i++)
//  {
////    t[i] = cycles();
//    val[i] = analogRead(25);    
//  }
//  Serial.println(millis()-t0);
//  delay(500);
//}

/*
 * This is an example to read analog data at high frequency using the I2S peripheral
 * Run a wire between pins 27 & 32
 * The readings from the device will be 12bit (0-4096) 
 */
#include <driver/i2s.h>

#define I2S_SAMPLE_RATE 100000
#define ADC_INPUT ADC1_CHANNEL_0 //pin SENSOR_VP
#define SAMPLE_NUM 10000

uint16_t adc_reading[SAMPLE_NUM];
float adc_results[SAMPLE_NUM/1000];
long adc_start_time = 0;
long adc_end_time = 0;

void i2sInit()
{
   i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate =  I2S_SAMPLE_RATE,              // The format of the signal using ADC_BUILT_IN
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1000,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
   };
   adc1_config_channel_atten(ADC1_CHANNEL_0,ADC_ATTEN_DB_0);
   adc1_config_width (ADC_WIDTH_12Bit);
   i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
   i2s_set_adc_mode(ADC_UNIT_1, ADC_INPUT);
   i2s_adc_enable(I2S_NUM_0);
}

int adc_read(uint16_t* adc_value){
  size_t num_bytes_read = 0;
  i2s_read( I2S_NUM_0, (void*) adc_value,  SAMPLE_NUM * sizeof (uint16_t), &num_bytes_read, portMAX_DELAY);
}

void setup() {
  Serial.begin(115200);
  // Initialize the I2S peripheral
  i2sInit();
}

void loop() {
  adc_start_time = millis();
  adc_read(adc_reading);
  adc_end_time = millis();
  Serial.print("adc cost: ");
  Serial.println(adc_end_time-adc_start_time);
  for (int i = 0; i < SAMPLE_NUM/1000; i++)
  {
    adc_results[i] = 1.1* ( (float) (adc_reading[i]& 0x0FFF)) /0x0FFF;
    Serial.println(adc_results[i]);
  }
  delay(10000);
}
