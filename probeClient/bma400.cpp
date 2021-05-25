#include "BMA400.h"

void BMA400::initialize(void) {
    if (setPoweMode(NORMAL) && setFullScaleRange(RANGE_4G) && setOutputDataRate(ODR_200)){
    }
}

bool BMA400::isConnection(void) {
    return (getDeviceID() == 0x90);
}

uint8_t BMA400::getDeviceID(void) {
    read8(BMA400_CHIPID);
    return read8(BMA400_CHIPID);
}

void BMA400::reset(void) {
    write8(BMA400_CMD, 0xb6);
}

bool BMA400::setPoweMode(power_type_t mode) {
    write8(BMA400_ACC_CONFIG_0, mode);
    return (read8(BMA400_ACC_CONFIG_0) == mode);
}

bool BMA400::setFullScaleRange(scale_type_t range) {

    if (range == RANGE_2G) {
        accRange = 2000;
    } else if (range == RANGE_4G) {
        accRange = 4000;
    } else if (range == RANGE_8G) {
        accRange = 8000;
    } else if (range == RANGE_16G) {
        accRange = 16000;
    }

    write8(BMA400_ACC_CONFIG_1, range);
    return (read8(BMA400_ACC_CONFIG_1) == range);
}

bool BMA400::setOutputDataRate(odr_type_t odr) {
    write8(BMA400_ACC_CONFIG_1, odr);
    return (read8(BMA400_ACC_CONFIG_1) == odr);
}

void BMA400::enable_shake_interrupt(){
    write8(0x21,0x04);
    write8(0x20,0x00);
    write8(0x24,0x02);
    write8(0x3F,0xFA);
    write8(0x40,0x01);
    write8(0x41,0x10);
    write8(0x42,0x00);
    write8(0x43,0x0F);
    write8(0x1F,0x04);
}

void BMA400::getAcceleration(float* x, float* y, float* z) {
    uint8_t buf[6] = {0};
    uint16_t ax = 0, ay = 0, az = 0;
    float value = 0;

    read(BMA400_ACC_X_LSB, buf, 6);

    ax = buf[0] | (buf[1] << 8);
    ay = buf[2] | (buf[3] << 8);
    az = buf[4] | (buf[5] << 8);

    if (ax > 2047) {
        ax = ax - 4096;
    }
    if (ay > 2047) {
        ay = ay - 4096;
    }
    if (az > 2047) {
        az = az - 4096;
    }

    value = (int16_t)ax;
    *x = accRange * value / 2048;

    value = (int16_t)ay;
    *y = accRange * value / 2048;

    value = (int16_t)az;
    *z = accRange * value / 2048;
}

float BMA400::getAccelerationX(void) {
    uint16_t ax = 0;
    float value = 0;

    ax = read16(BMA400_ACC_X_LSB);
    if (ax > 2047) {
        ax = ax - 4096;
    }

    value = (int16_t)ax;
    value = accRange * value / 2048;

    return value;
}

float BMA400::getAccelerationY(void) {
    uint16_t ay = 0;
    float value = 0;

    ay = read16(BMA400_ACC_Y_LSB);
    if (ay > 2047) {
        ay = ay - 4096;
    }

    value = (int16_t)ay;
    value = accRange * value / 2048;

    return value;
}

float BMA400::getAccelerationZ(void) {
    uint16_t az = 0;
    float value = 0;

    az = read16(BMA400_ACC_Z_LSB);
    if (az > 2047) {
        az = az - 4096;
    }

    value = (int16_t)az;
    value = accRange * value / 2048;

    return value;
}

int16_t BMA400::getTemperature(void) {
    int8_t data = 0;
    int16_t temp = 0;

    data = (int8_t)read8(BMA400_TEMP_DATA);
    temp = data / 2 + 23;

    return temp;
}

void BMA400::write8(uint8_t reg, uint8_t val) {
    uint16_t D = ((reg << 8) | val);
    _hspi->beginTransaction(SPISettings());
    digitalWrite(HSPICS0,LOW);
    _hspi->write16(D);
    digitalWrite(HSPICS0,HIGH);
    _hspi->endTransaction();
}

uint8_t BMA400::read8(uint8_t reg) {
    uint32_t R = ((reg | 0x80) << 24);
    _hspi->beginTransaction(SPISettings());
    digitalWrite(HSPICS0,LOW);
    uint32_t D = _hspi->transfer32(R);
    digitalWrite(HSPICS0,HIGH);
    _hspi->endTransaction();
    uint8_t data = ((D & 0x0000FF00) >> 8);
    return data;
}

uint16_t BMA400::read16(uint8_t reg) {
    uint32_t R = ((reg | 0x80) << 24);
    _hspi->beginTransaction(SPISettings());
    digitalWrite(HSPICS0,LOW);
    uint32_t D = _hspi->transfer32(R);
    digitalWrite(HSPICS0,HIGH);
    _hspi->endTransaction();
    uint16_t data = (D & 0x0000FFFF);
    return data;
}

void BMA400::read(uint8_t reg, uint8_t* buf, uint16_t len) {
    buf[0] = reg;
    _hspi->beginTransaction(SPISettings());
    digitalWrite(HSPICS0,LOW);
    _hspi->transfer(buf,len);
    digitalWrite(HSPICS0,HIGH);
    _hspi->endTransaction();
}
