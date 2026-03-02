/*
 * Gee-Force: Model Rocket Launch G-Force Tracker
 *
 * Hardware:
 * - Particle Photon microcontroller
 * - Grove 4-digit display (CLK: D2, DIO: D3)
 * - Grove 3-axis digital accelerometer (I2C, MMA7660 chip)
 *
 * Note: MMA7660 accelerometer is for prototyping only (limited range)
 */

#include "Particle.h"
#include "Grove_4Digit_Display.h"
#include "MMA7660-Accelerometer.h"

// Pin definitions
#define CLK D2
#define DIO D3

// Hardware instances
TM1637 display(CLK, DIO);
MMA7660 accelerometer;

// G-force tracking
float maxGForce = 0.0;
float currentGForce = 0.0;

void setup() {
    Serial.begin(9600);

    // Initialize display
    display.init();
    display.set(BRIGHT_TYPICAL);
    display.point(POINT_OFF);

    // Initialize accelerometer
    accelerometer.init();

    Serial.println("Gee-Force Tracker Initialized");
    displayValue(0.0);
}

void loop() {
    // Read accelerometer data
    int8_t x, y, z;
    accelerometer.getXYZ(&x, &y, &z);

    // Calculate total G-force magnitude
    // MMA7660 returns values in units of 1/21.3 g per LSB
    float ax = x / 21.3;
    float ay = y / 21.3;
    float az = z / 21.3;

    currentGForce = sqrt(ax*ax + ay*ay + az*az);

    // Track maximum G-force
    if (currentGForce > maxGForce) {
        maxGForce = currentGForce;
    }

    // Display current G-force
    displayValue(currentGForce);

    // Publish to Particle Cloud (optional, for data logging)
    Particle.publish("gforce", String::format("%.2f", currentGForce), PRIVATE);

    delay(100); // 10Hz update rate
}

void displayValue(float value) {
    // Display G-force value (e.g., 1.23 shows as "1.23")
    int displayValue = (int)(value * 100); // Convert to integer (123 for 1.23g)

    int digit1 = (displayValue / 1000) % 10;
    int digit2 = (displayValue / 100) % 10;
    int digit3 = (displayValue / 10) % 10;
    int digit4 = displayValue % 10;

    display.display(0, digit1);
    display.display(1, digit2);
    display.display(2, digit3);
    display.display(3, digit4);
    display.point(POINT_ON); // Show decimal point between digits 1 and 2
}
