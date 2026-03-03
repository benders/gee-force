/**
 * NewRelicClient implementation - Webhook-based approach
 */

#include "NewRelicClient.h"

NewRelicClient::NewRelicClient(const char* deviceId)
    : sampleCount(0), publishCount(0), failureCount(0), samplesDropped(0) {

    // Copy device ID
    strncpy(this->deviceId, deviceId, sizeof(this->deviceId) - 1);
    this->deviceId[sizeof(this->deviceId) - 1] = '\0';
}

bool NewRelicClient::addSample(const AccelSample& sample) {
    if (isFull()) {
        samplesDropped++;
        return false;
    }

    samples[sampleCount++] = sample;
    return true;
}

void NewRelicClient::clear() {
    sampleCount = 0;
}

bool NewRelicClient::buildJsonPayload(char* buffer, int bufferSize) {
    // Build JSON array of events
    // Format: [{"eventType":"AccelSample","timestamp":123,...},...]

    int offset = 0;

    // Start array
    offset += snprintf(buffer + offset, bufferSize - offset, "[");

    // Add each sample
    for (int i = 0; i < sampleCount; i++) {
        bool isLast = (i == sampleCount - 1);
        int written = appendSampleJson(buffer, offset, bufferSize, samples[i], isLast);
        if (written < 0) {
            return false; // Buffer overflow
        }
        offset += written;
    }

    // End array
    offset += snprintf(buffer + offset, bufferSize - offset, "]");

    return (offset < bufferSize);
}

int NewRelicClient::appendSampleJson(char* buffer, int offset, int maxSize,
                                      const AccelSample& sample, bool isLast) {
    // Build single event JSON
    // {"eventType":"AccelSample","timestamp":123,"accelX":0.1,"accelY":0.2,"accelZ":0.3,"magnitude":0.4,"deviceId":"xyz"}

    // Format timestamp as string to avoid %llu printf issues on this platform
    char timestampStr[32];
    snprintf(timestampStr, sizeof(timestampStr), "%lu%03lu",
             (unsigned long)(sample.timestamp / 1000),
             (unsigned long)(sample.timestamp % 1000));

    int remaining = maxSize - offset;
    int written = snprintf(buffer + offset, remaining,
        "{\"eventType\":\"AccelSample\","
        "\"timestamp\":%s,"
        "\"accelX\":%.3f,"
        "\"accelY\":%.3f,"
        "\"accelZ\":%.3f,"
        "\"magnitude\":%.3f,"
        "\"deviceId\":\"%s\"}%s",
        timestampStr,
        sample.accelX,
        sample.accelY,
        sample.accelZ,
        sample.magnitude,
        deviceId,
        isLast ? "" : ","
    );

    if (written >= remaining) {
        return -1; // Buffer overflow
    }

    return written;
}

bool NewRelicClient::sendBatch() {
    if (sampleCount == 0) {
        return true; // Nothing to send
    }

    // Allocate buffer for JSON payload
    // Particle.publish() has a 622 byte limit for data
    // With 4 samples at ~150 bytes each, we stay under 620 bytes

    static char payload[620]; // Leave 2 bytes for safety

    if (!buildJsonPayload(payload, sizeof(payload))) {
        Serial.println("NR: JSON build failed (buffer overflow)");
        Serial.printlnf("NR: Try reducing NR_MAX_SAMPLES (currently %d)", sampleCount);
        failureCount++;
        clear();
        return false;
    }

    int payloadLength = strlen(payload);

    // Check if payload fits in Particle.publish() limit (622 bytes)
    if (payloadLength > 620) {
        Serial.printlnf("NR: Payload too large (%d bytes, max 620)", payloadLength);
        Serial.println("NR: Consider reducing NR_MAX_SAMPLES");
        failureCount++;
        clear();
        return false;
    }

    Serial.printlnf("NR: Publishing %d samples (%d bytes)", sampleCount, payloadLength);

    // Publish via Particle webhook
    // Event name matches webhook configuration
    // Data is JSON array of samples
    // bool success = Particle.publish(NR_EVENT_NAME, payload, PRIVATE);
    bool success = 0;

    if (success) {
        Serial.printlnf("NR: Published successfully (%d samples)", sampleCount);
        publishCount++;
    } else {
        Serial.println("NR: Publish failed (cloud not connected or rate limited?)");
        failureCount++;
    }

    clear();
    return success;
}
