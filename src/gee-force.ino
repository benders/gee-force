// This #include statement was automatically added by the Particle IDE.
#include <Grove_4Digit_Display.h>
#define CLK D2
#define DIO D3
TM1637 tm1637(CLK, DIO);

#include "Particle.h"
#include "MQTTManager.h"

// Let Device OS manage the connection to the Particle Cloud.
// SYSTEM_THREAD(ENABLED) is intentionally omitted: the MQTT library uses
// TCPClient directly; concurrent system-thread WiFi management causes hard
// faults on the Photon.  AUTOMATIC mode without threading is stable.
SYSTEM_MODE(AUTOMATIC);

// Enable application watchdog (60 second timeout).
ApplicationWatchdog wd(60000, System.reset);

// Show system, cloud connectivity, and application logs over USB.
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);

#include "MMA7660.h"
MMA7660 accel;
float accelX, accelY, accelZ;
float mag = 0.0f;

unsigned long lastSample = 0;
unsigned long lastStatus  = 0;

#define SAMPLE_INTERVAL  333    // accelerometer sampling interval (ms) — ~3 Hz
#define STATUS_INTERVAL  30000  // MQTT status heartbeat interval (ms) — 0.033 Hz

// Global MQTT manager — stack-allocated, no heap allocation after setup.
MQTTManager mqttManager;

// Device ID stored as a static char array (no String in the hot path).
static char deviceId[32];

bool timeIsSynced = false;

// ---------------------------------------------------------------------------
// Display helper
// ---------------------------------------------------------------------------
void displayNum(TM1637 tm, float num, int decimal = 0, bool show_minus = true) {
    const int DIGITS = 4;
    int number = (int)round(fabs(num) * pow(10, decimal));

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
            tm.display(j, 0x7f);  // blank digit
        }
        number /= 10;
    }

    if (show_minus && num < 0) {
        tm.display(0, '-');
    }
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
    waitFor(Serial.isConnected, 5000);

    if (!accel.begin()) {
        Serial.println("ERROR: Could not initialise accelerometer. Halting.");
        while (true) { Particle.process(); }
    }

    tm1637.init();
    tm1637.set(BRIGHT_TYPICAL);
    tm1637.point(POINT_OFF);

    // Capture device ID into static buffer — no String heap allocation later.
    String idStr = System.deviceID();
    idStr.toCharArray(deviceId, sizeof(deviceId));

    Serial.printlnf("Device ID: %s", deviceId);

    // Wait for time synchronization before publishing timestamped samples.
    Serial.println("Waiting for time sync...");
    Particle.syncTime();
    waitFor(Particle.syncTimeDone, 30000);

    if (Time.isValid()) {
        timeIsSynced = true;
        Serial.printlnf("Time synced: %s",
            Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL).c_str());
    } else {
        Serial.println("WARNING: Time sync failed — samples will not be published until synced");
    }

    // Initialize MQTT manager (connects to broker).
    mqttManager.begin(deviceId);

    Log.info("Setup complete");
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
void loop() {
    // Pet the watchdog to prevent reset.
    ApplicationWatchdog::checkin();

    unsigned long now = millis();

    // Drive MQTT reconnect logic and keep-alive.
    mqttManager.loop();

    // --- Sample accelerometer and publish every SAMPLE_INTERVAL ms ---
    if (now - lastSample >= SAMPLE_INTERVAL) {
        lastSample = now;

        if (accel.readAxes(accelX, accelY, accelZ)) {
            mag = accel.magnitude(accelX, accelY, accelZ);

            displayNum(tm1637, mag, 2);

            Serial.printlnf("Accel  X: %+.3fg  Y: %+.3fg  Z: %+.3fg  |a|: %.3fg",
                            accelX, accelY, accelZ, mag);

            if (timeIsSynced) {
                // Build millisecond-precision timestamp using 64-bit arithmetic.
                unsigned long long ts =
                    ((unsigned long long)Time.now()) * 1000ULL + (millis() % 1000);

                mqttManager.publishSample(ts, accelX, accelY, accelZ, mag);
            }
        }
    }

    // --- Publish status heartbeat every STATUS_INTERVAL ms ---
    if (now - lastStatus >= STATUS_INTERVAL) {
        lastStatus = now;

        int rssi = WiFi.RSSI();
        mqttManager.publishStatus(now / 1000, rssi);

        Log.info("Status: uptime=%lus  rssi=%d  mqttFails=%lu  connected=%s",
                 now / 1000,
                 rssi,
                 mqttManager.publishFailures(),
                 mqttManager.isConnected() ? "yes" : "no");
    }

    Particle.process();
}
