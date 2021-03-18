#include <Arduino.h>
#include "SPI.h"
#include "bma400.h"

#define INT1PIN 36

float x = 0, y = 0, z = 0;
SPIClass* hspi = new SPIClass(HSPI);
BMA400 bma400(hspi);
bool irFlag = false;

void IRAM_ATTR isr()
{
    irFlag = true;
}

void setup()
{
    hspi->begin();
    pinMode(15, OUTPUT);
    pinMode(36, INPUT);
    Serial.begin(115200);
    attachInterrupt(INT1PIN, isr, FALLING);
    
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
}

void loop()
{
    // bma400.getAcceleration(&x, &y, &z);
    // Serial.println(x);
//    Serial.print(",");
//    Serial.print(y);
//    Serial.print(",");
//    Serial.print(z);
//    Serial.println(",");
    if (irFlag)
    {
        irFlag = false;
        Serial.println("monitor triggerred!");
    }
}
