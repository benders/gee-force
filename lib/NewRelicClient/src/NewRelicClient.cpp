/**
 * NewRelicClient implementation
 */

#include "NewRelicClient.h"

NewRelicClient::NewRelicClient(const char* accountId, const char* insertKey, const char* deviceId)
    : sampleCount(0), successCount(0), failureCount(0), samplesDropped(0) {

    // Copy configuration strings
    strncpy(this->accountId, accountId, sizeof(this->accountId) - 1);
    this->accountId[sizeof(this->accountId) - 1] = '\0';

    strncpy(this->insertKey, insertKey, sizeof(this->insertKey) - 1);
    this->insertKey[sizeof(this->insertKey) - 1] = '\0';

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

    int remaining = maxSize - offset;
    int written = snprintf(buffer + offset, remaining,
        "{\"eventType\":\"AccelSample\","
        "\"timestamp\":%lu,"
        "\"accelX\":%.3f,"
        "\"accelY\":%.3f,"
        "\"accelZ\":%.3f,"
        "\"magnitude\":%.3f,"
        "\"deviceId\":\"%s\"}%s",
        sample.timestamp,
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

bool NewRelicClient::sendHttpPost(const char* payload, int payloadLength) {
    // Build URL path
    char urlPath[128];
    snprintf(urlPath, sizeof(urlPath), "/v1/accounts/%s/events", accountId);

    // Connect to New Relic
    if (!client.connect(NR_HOSTNAME, NR_PORT)) {
        Serial.println("NR: Connection failed");
        return false;
    }

    // Build and send HTTP request
    // Note: Using HTTP for now. For production, consider using Particle webhooks for HTTPS
    client.print("POST ");
    client.print(urlPath);
    client.println(" HTTP/1.1");

    client.print("Host: ");
    client.println(NR_HOSTNAME);

    client.print("X-Insert-Key: ");
    client.println(insertKey);

    client.println("Content-Type: application/json");

    client.print("Content-Length: ");
    client.println(payloadLength);

    client.println("Connection: close");
    client.println();

    // Send payload
    client.print(payload);

    // Read response
    unsigned long startTime = millis();
    bool success = false;

    while (client.connected() && (millis() - startTime < NR_TIMEOUT)) {
        if (client.available()) {
            String line = client.readStringUntil('\n');

            // Check for HTTP 200 OK response
            if (line.indexOf("HTTP/1.1 200") >= 0 || line.indexOf("HTTP/1.1 201") >= 0) {
                success = true;
            }

            // Debug: print first line of response
            if (line.length() > 0) {
                Serial.print("NR Response: ");
                Serial.println(line);
                break; // Only read first line for efficiency
            }
        }
    }

    client.stop();
    return success;
}

bool NewRelicClient::sendBatch() {
    if (sampleCount == 0) {
        return true; // Nothing to send
    }

    // Allocate buffer for JSON payload
    // Estimate: ~120 bytes per sample × 20 samples = 2400 bytes + overhead = 2500 bytes
    static char payload[2600];

    if (!buildJsonPayload(payload, sizeof(payload))) {
        Serial.println("NR: JSON build failed (buffer overflow)");
        failureCount++;
        clear();
        return false;
    }

    int payloadLength = strlen(payload);
    Serial.printlnf("NR: Sending %d samples (%d bytes)", sampleCount, payloadLength);

    bool success = sendHttpPost(payload, payloadLength);

    if (success) {
        Serial.printlnf("NR: Batch sent successfully (%d samples)", sampleCount);
        successCount++;
    } else {
        Serial.println("NR: Batch send failed");
        failureCount++;
    }

    clear();
    return success;
}
