#include "Arduino.h"
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "as3933.h"
#include <SPI.h>

static const int spiClk = 1000000;
SPIClass *hspi = NULL;

void setup()
{
    hspi = new SPIClass(HSPI);
    //initialise hspi with default pins
    //SCLK = 14, MISO = 12, MOSI = 13, SS = 15
    hspi->begin();
    //alternatively route through GPIO pins
    //hspi->begin(25, 26, 27, 32); //SCLK, MISO, MOSI, SS
    pinMode(15, OUTPUT); //HSPI SS
}

void loop(){
    as3933_directcmd(hspi,CLEAR_WAKE);
    delay(10);
}
