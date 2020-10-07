#include <esp_spi_flash.h>
#include <esp_attr.h>

size_t addr = 0x200000;
esp_err_t err;
char tbuff[] = "this is a test";
char rbuff[SPI_FLASH_SEC_SIZE / 4];
void setup() {
  spi_flash_init();
  int fsize = spi_flash_get_chip_size();
  Serial.begin(115200);
  Serial.println("flash ready");
  Serial.println(fsize);
  spi_flash_erase_sector(addr / SPI_FLASH_SEC_SIZE);
  err = spi_flash_write(addr,tbuff,sizeof(tbuff));
  Serial.println("data written");
  err = spi_flash_read(addr,rbuff, sizeof(tbuff));
  Serial.println(rbuff);
}

void loop() {

}
