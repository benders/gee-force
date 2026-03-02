#pragma once

// Include Particle Device OS APIs
#include "Particle.h"

// MMA7660 I2C address (ADDR pin low = 0x4C, high = 0x4D)
#define MMA7660_ADDR    0x4C

// MMA7660 Register addresses
#define REG_XOUT        0x00
#define REG_YOUT        0x01
#define REG_ZOUT        0x02
#define REG_MODE        0x07
#define REG_SR          0x08  // Sample Rate register

// Mode register bits
#define MODE_ACTIVE     0x01
#define MODE_STANDBY    0x00

// The MMA7660 returns 6-bit signed values (range -32 to +31)
// At default ±1.5g range, each count = 0.047g
#define COUNTS_PER_G    21.33f

class MMA7660 {
public:
    // Initialise the device and activate it.
    // Returns true on success.
    bool begin();

    // Read all three axes into x, y, z (in g).
    // Retries automatically if the alert bit is set.
    // Returns true on success.
    bool readAxes(float &x, float &y, float &z);

    // Compute vector magnitude: sqrt(x² + y² + z²).
    // A stationary device should return ~1.0g.
    float magnitude(float x, float y, float z);

private:
    bool    writeRegister(uint8_t reg, uint8_t value);
    int16_t readRegister(uint8_t reg);
    int8_t  rawToSigned(uint8_t raw);
};
