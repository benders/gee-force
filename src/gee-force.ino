// This #include statement was automatically added by the Particle IDE.
#include <Grove_4Digit_Display.h>
#define CLK D2 //pins definitions for TM1637 and can be changed to other ports
#define DIO D3
TM1637 tm1637(CLK,DIO);

// Include Particle Device OS APIs
#include "Particle.h"

// New Relic client
#include "NewRelicClient.h"

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
unsigned long lastTransmit = 0; // New Relic batch transmission timer

#define SAMPLE_INTERVAL   333   // accelerometer sampling (ms) - ~3Hz
#define TRANSMIT_INTERVAL 1000  // New Relic batch transmission (ms) - 1Hz
                                // (3 samples per batch = 1000ms, stays within rate limits)

// Device ID (use Particle device ID)
String deviceIdStr;
NewRelicClient* nrClient = nullptr;
bool timeIsSynced = false;  // Flag to prevent sampling until time is synced

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
    // Wait for serial connection (for debugging)
    // Remove or comment out for production
    waitFor(Serial.isConnected, 5000);

    if (!accel.begin()) {
        Serial.println("ERROR: Could not initialise accelerometer. Halting.");
        while (true) { Particle.process(); }
    }

    tm1637.init();
    tm1637.set(BRIGHT_TYPICAL); //BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
    tm1637.point(POINT_OFF);

    // Initialize New Relic client (uses Particle webhooks)
    deviceIdStr = System.deviceID();
    nrClient = new NewRelicClient(deviceIdStr.c_str());

    Serial.printlnf("Device ID: %s", deviceIdStr.c_str());
    Serial.println("New Relic client initialized (webhook mode)");
    Serial.println("Remember to configure the 'nr_accel' webhook in Particle Console");

    // Wait for time synchronization
    Serial.println("Waiting for time sync...");
    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 30000);

    if (Time.isValid()) {
        Serial.printlnf("Time synced: %s", Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL).c_str());
        unsigned long now = Time.now();
        Serial.printlnf("Unix timestamp: %lu seconds = %lu%03lu milliseconds",
                        now, now, (unsigned long)((millis() % 1000)));
        timeIsSynced = true;
    } else {
        Serial.println("WARNING: Time sync failed, timestamps may be incorrect");
        Serial.println("Samples will NOT be sent to New Relic until time is synced");
    }

    Log.info("Setup has finished!");
}

// loop() runs over and over again, as quickly as it can execute.
// This is a very basic programming model (the superloop) -
// for more advanced threading models, please see the Device OS documentation at docs.particle.io
void loop() {
    // Pet the watchdog to prevent reset
    ApplicationWatchdog::checkin();

    unsigned long now = millis();

    // --- Fast path: read accelerometer and buffer samples every 50ms (20Hz) ---
    if (now - lastSample >= SAMPLE_INTERVAL) {
        lastSample = now;

        if (accel.readAxes(accelX, accelY, accelZ)) {
            mag = accel.magnitude(accelX, accelY, accelZ);

            // Update the 4-digit display on every sample
            displayNum(tm1637, mag, 2);

            // Buffer sample for New Relic (only if time is synced)
            if (nrClient && timeIsSynced) {
                AccelSample sample;
                // Use 64-bit arithmetic to avoid overflow: Unix timestamp in milliseconds
                sample.timestamp = ((unsigned long long)Time.now()) * 1000ULL + (millis() % 1000);
                sample.accelX = accelX;
                sample.accelY = accelY;
                sample.accelZ = accelZ;
                sample.magnitude = mag;

                if (!nrClient->addSample(sample)) {
                    Serial.println("NR: Buffer full, sample dropped");
                }
            }

            // Console output on every sample
            Serial.printlnf("Accel  X: %+.3fg  Y: %+.3fg  Z: %+.3fg  |a|: %.3fg",
                            accelX, accelY, accelZ, mag);
        }
    }

    // --- Slow path: transmit batched samples to New Relic every 1000ms ---
    if (now - lastTransmit >= TRANSMIT_INTERVAL) {
        lastTransmit = now;

        if (nrClient && nrClient->getSampleCount() > 0) {
            Serial.printlnf("NR: Transmitting batch of %d samples", nrClient->getSampleCount());
            Serial.printlnf("NR: Current Time.now() = %lu seconds", Time.now());

            bool success = nrClient->sendBatch();

            if (success) {
                Serial.printlnf("NR Stats - Published: %lu, Failed: %lu, Dropped: %lu",
                    nrClient->getPublishCount(),
                    nrClient->getFailureCount(),
                    nrClient->getSamplesDropped());
            }
        }
    }

    Particle.process();
}
