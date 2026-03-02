// This #include statement was automatically added by the Particle IDE.
#include <Grove_4Digit_Display.h>
#define CLK D2 //pins definitions for TM1637 and can be changed to other ports
#define DIO D3
TM1637 tm1637(CLK,DIO);

// Include Particle Device OS APIs
#include "Particle.h"

// This #include statement was automatically added by the Particle IDE.
// #include <MMA7660-Accelerometer.h>
// MMA7660 accelerometer;

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Enable application watchdog (60 second timeout)
ApplicationWatchdog wd(60000, System.reset);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

#include "MMA7660.h"
MMA7660 accel;
float accelX, accelY, accelZ;
float mag = 0.0f;               // promoted so both timers can access it
unsigned long lastSample  = 0;
unsigned long lastPublish = 0;  // independent publish timer

#define SAMPLE_INTERVAL   100   // accelerometer + display refresh (ms)
#define PUBLISH_INTERVAL  2000  // Particle cloud publish (ms)

void displayNum(TM1637 tm, float num, int decimal = 0, bool show_minus = true) {
    // Displays number with decimal places (no decimal point implementation)
    // Colon is used instead of decimal point if decimal == 2
    // Be aware of int size limitations (up to +-2^15 = +-32767)
    const int DIGITS = 4; // Number of digits on display

    int number = round(fabs(num) * pow(10, decimal));

    if (decimal == 2) {
        tm.point(true);
    } else {
        tm.point(false);
    }

    for (int i = 0; i < DIGITS - (show_minus && num < 0 ? 1 : 0); ++i) {
        int j = DIGITS - i - 1;

        if (number != 0) {
            tm.display(j, number % 10);
        } else {
            tm.display(j, 0x7f);    // display nothing
        }

        number /= 10;
    }

    if (show_minus && num < 0) {
        tm.display(0, '-');    // Display '-'
    }
}

// setup() runs once, when the device is first turned on
void setup() {
    if (!accel.begin()) {
        Serial.println("ERROR: Could not initialise accelerometer. Halting.");
        while (true) { Particle.process(); }
    }

    tm1637.init();
    tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
    tm1637.point(POINT_OFF);

    Log.info("Setup has finished!");
}

// loop() runs over and over again, as quickly as it can execute.
// This is a very basic programming model (the superloop) - 
// for more advanced threading models, please see the Device OS documentation at docs.particle.io
void loop() {
    // Pet the watchdog to prevent reset
    ApplicationWatchdog::checkin();

    unsigned long now = millis();

    // --- Fast path: read accelerometer and update display every 100ms ---
    if (now - lastSample >= SAMPLE_INTERVAL) {
        lastSample = now;

        if (accel.readAxes(accelX, accelY, accelZ)) {
            mag = accel.magnitude(accelX, accelY, accelZ);

            // Update the 4-digit display on every sample
            displayNum(tm1637, mag, 2);

            // Console output on every sample
            Serial.printlnf("Accel  X: %+.3fg  Y: %+.3fg  Z: %+.3fg  |a|: %.3fg",
                            accelX, accelY, accelZ, mag);
        }
    }

    // --- Slow path: publish to Particle cloud every 2000ms ---
    if (now - lastPublish >= PUBLISH_INTERVAL) {
        lastPublish = now;

        char payload[64];
        snprintf(payload, sizeof(payload), "%.3f", mag);

        bool published = Particle.publish("accel_magnitude", payload,
                                          60, PRIVATE);
        if (published) {
            Serial.printlnf("Published magnitude: %sg", payload);
        } else {
            Serial.println("Publish failed (cloud not connected?)");
        }
    }

    Particle.process();
}
