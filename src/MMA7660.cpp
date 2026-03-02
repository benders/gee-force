#include "MMA7660.h"
#include <math.h>

bool MMA7660::begin() {
    Wire.begin();

    if (!writeRegister(REG_MODE, MODE_STANDBY)) {
        Serial.println("Failed to contact MMA7660");
        return false;
    }

    // 32 samples/sec
    writeRegister(REG_SR, 0x03);

    if (!writeRegister(REG_MODE, MODE_ACTIVE)) {
        Serial.println("Failed to activate MMA7660");
        return false;
    }

    delay(20);
    Serial.println("MMA7660 initialised successfully");
    return true;
}

bool MMA7660::readAxes(float &x, float &y, float &z) {
    const int MAX_RETRIES = 5;

    for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
        int16_t rawX = readRegister(REG_XOUT);
        int16_t rawY = readRegister(REG_YOUT);
        int16_t rawZ = readRegister(REG_ZOUT);

        if (rawX < 0 || rawY < 0 || rawZ < 0) {
            Serial.println("I2C read error");
            return false;
        }

        if ((rawX & 0x40) || (rawY & 0x40) || (rawZ & 0x40)) {
            delayMicroseconds(200);
            continue;
        }

        x = rawToSigned((uint8_t)rawX) / COUNTS_PER_G;
        y = rawToSigned((uint8_t)rawY) / COUNTS_PER_G;
        z = rawToSigned((uint8_t)rawZ) / COUNTS_PER_G;
        return true;
    }

    Serial.println("Alert bit persistently set — data unreliable");
    return false;
}

float MMA7660::magnitude(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

// --- Private methods ---

bool MMA7660::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MMA7660_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

int16_t MMA7660::readRegister(uint8_t reg) {
    Wire.beginTransmission(MMA7660_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return -1;

    Wire.requestFrom(MMA7660_ADDR, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return -1;
}

int8_t MMA7660::rawToSigned(uint8_t raw) {
    raw &= 0x3F;
    if (raw & 0x20) {
        return (int8_t)(raw | 0xC0);
    }
    return (int8_t)raw;
}
