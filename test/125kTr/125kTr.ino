#include <Arduino.h>

#define SERIAL_DEBUG

#define Pinh1 25
#define Pinh2 27

void setup()
{
    pinMode(Pinh1,OUTPUT);
    digitalWrite(Pinh1,LOW);
#ifdef SERIAL_DEBUG
    Serial.begin(115200);
#endif
    uint32_t freq = ledcSetup(0, 125000, 4); //channel, freq, resolution
#ifdef SERIAL_DEBUG
    Serial.printf("Output frequency: %d\n", freq);
#endif
    ledcAttachPin(Pinh2, 0); //pin, channel
}

void loop(){
    ledcWrite(0, 8);    //channel, duty
    digitalWrite(Pinh1,HIGH);
    delay(100);
    digitalWrite(Pinh1,LOW);
    ledcWrite(0, 0);    //channel, duty
    delay(10000);
    Serial.println("trigger sent");
    Serial.println("something changed");
}
