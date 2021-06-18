#include "as3933.h"

void hspiCommand(SPIClass *hspi, byte data)
{
    hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    digitalWrite(15, LOW);
    hspi->transfer(data);
    digitalWrite(15, HIGH);
    hspi->endTransaction();
}

esp_err_t as3933_directcmd(SPIClass *hspi, uint_8 cmd)
{
    try
    {
        hspiCommand(hspi, cmd);
    }
    catch(...){
        return ESP_ERR_NOT_FOUND
    }
    return ESP_OK
}


