#pragma once

#include "Particle.h"
#include "credentials.h"
#include "MQTT.h"

// No-op callback required by the MQTT library for subscribe support.
// Must have external linkage so it can be passed as a function pointer.
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Not subscribing to any topics; nothing to do.
    (void)topic;
    (void)payload;
    (void)length;
}

class MQTTManager {
public:
    MQTTManager()
        : _client(nullptr),
          _publishFailures(0),
          _lastReconnectAttempt(0),
          _reconnectInterval(1000)
    {
        _topicSample[0]  = '\0';
        _topicStatus[0]  = '\0';
        _clientId[0]     = '\0';
    }

    // Call once from setup() after deviceId is known.
    // Constructs the MQTT client here (after RTOS is running) to avoid
    // os_mutex_create being called in a global constructor before the scheduler starts.
    void begin(const char* deviceId) {
        snprintf(_clientId,    sizeof(_clientId),    "geeforce-%s", deviceId);
        snprintf(_topicSample, sizeof(_topicSample), "geeforce/%s/accel",  deviceId);
        snprintf(_topicStatus, sizeof(_topicStatus), "geeforce/%s/status", deviceId);

        Log.info("MQTT: broker=%s:%d clientId=%s", MQTT_BROKER_HOST, MQTT_BROKER_PORT, _clientId);
        Log.info("MQTT: topic sample=%s  status=%s", _topicSample, _topicStatus);

        // Construct MQTT client now (inside setup(), after RTOS is running).
        _client = new MQTT(MQTT_BROKER_HOST, MQTT_BROKER_PORT, mqttCallback);

        _connect();
    }

    // Call every loop iteration.
    void loop() {
        if (!_client) return;
        if (_client->isConnected()) {
            _client->loop();
            _reconnectInterval = 1000; // reset backoff on confirmed connected state
        } else {
            unsigned long now = millis();
            if (now - _lastReconnectAttempt >= _reconnectInterval) {
                _lastReconnectAttempt = now;
                Log.info("MQTT: not connected, attempting reconnect (backoff=%lums)", _reconnectInterval);
                _connect();
                // Exponential backoff capped at 60 s.
                _reconnectInterval = (_reconnectInterval * 2 > 60000UL) ? 60000UL : _reconnectInterval * 2;
            }
        }
    }

    // Publish a single accelerometer sample.
    // Returns true on success.
    bool publishSample(unsigned long long ts, float x, float y, float z, float magnitude) {
        Log.info("DBG publish: enter freeMem=%lu connected=%d",
                 (unsigned long)System.freeMemory(), (int)_client->isConnected());

        if (!_client->isConnected()) {
            Log.warn("MQTT: publishSample skipped — not connected");
            _publishFailures++;
            return false;
        }

        static char buf[256];
        // newlib-nano (--specs=nano.specs) does not support %llu.
        // Split the 64-bit timestamp into seconds + milliseconds and print as
        // two adjacent %lu fields so both parts fit in 32 bits.
        unsigned long ts_s  = (unsigned long)(ts / 1000ULL);
        unsigned int  ts_ms = (unsigned int)(ts % 1000ULL);
        Log.info("DBG publish: ts_s=%lu ts_ms=%u", ts_s, ts_ms);

        int n = snprintf(buf, sizeof(buf),
            "{\"ts\":%lu%03u,\"x\":%.3f,\"y\":%.3f,\"z\":%.3f,\"m\":%.3f,\"dev\":\"%s\"}",
            ts_s, ts_ms, x, y, z, magnitude, _clientId);
        Log.info("DBG publish: snprintf n=%d buf=%.80s", n, buf);

        Log.info("DBG publish: calling client.publish topic=%s", _topicSample);
        bool ok = _client->publish(_topicSample, buf);
        Log.info("DBG publish: publish returned %d", (int)ok);

        if (!ok) {
            Log.warn("MQTT: publishSample failed (total failures: %lu)", _publishFailures + 1);
            _publishFailures++;
        }
        return ok;
    }

    // Publish a status heartbeat.
    // Returns true on success.
    bool publishStatus(unsigned long uptime, int rssi) {
        if (!_client->isConnected()) {
            Log.warn("MQTT: publishStatus skipped — not connected");
            return false;
        }

        static char buf[128];
        snprintf(buf, sizeof(buf), "{\"uptime\":%lu,\"rssi\":%d}", uptime, rssi);

        bool ok = _client->publish(_topicStatus, buf);
        if (!ok) {
            Log.warn("MQTT: publishStatus failed");
        }
        return ok;
    }

    bool isConnected() { return _client && _client->isConnected(); }
    unsigned long publishFailures() const { return _publishFailures; }

private:
    MQTT* _client;

    char _topicSample[80];
    char _topicStatus[80];
    char _clientId[60];

    unsigned long _publishFailures;
    unsigned long _lastReconnectAttempt;
    unsigned long _reconnectInterval;   // ms, grows with backoff

    void _connect() {
        bool ok = _client->connect(_clientId, MQTT_USERNAME, MQTT_PASSWORD);
        if (ok) {
            Log.info("MQTT: connected as %s", _clientId);
            _reconnectInterval = 1000;
        } else {
            Log.warn("MQTT: connect failed");
        }
    }
};
