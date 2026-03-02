/**
 * NewRelicClient - Batches accelerometer samples for New Relic via Particle Webhooks
 *
 * Sends batched accelerometer samples to New Relic using Particle.publish() and webhooks.
 * Credentials are stored securely in the webhook configuration, not on the device.
 *
 * Features:
 * - Static memory allocation (no heap fragmentation)
 * - Batched transmission (reduces publish overhead and rate limits)
 * - Simple JSON serialization (no external library needed)
 * - Secure: No credentials on device or in code
 * - Automatic HTTPS via Particle Cloud
 */

#ifndef NEW_RELIC_CLIENT_H
#define NEW_RELIC_CLIENT_H

#include "Particle.h"

// Configuration
// Note: Particle.publish() has a 622 byte data limit
// Each sample is ~120 bytes in JSON format
// 5 samples = ~600 bytes (safe margin)
#define NR_MAX_SAMPLES 5               // Max samples per batch (limited by Particle.publish)
#define NR_EVENT_NAME "nr_accel"       // Particle event name for webhook

// Sample data structure
struct AccelSample {
    unsigned long timestamp;  // Milliseconds since epoch
    float accelX;            // X-axis acceleration (g)
    float accelY;            // Y-axis acceleration (g)
    float accelZ;            // Z-axis acceleration (g)
    float magnitude;         // Combined magnitude (g)
};

class NewRelicClient {
public:
    /**
     * Constructor
     * @param deviceId Unique device identifier
     */
    NewRelicClient(const char* deviceId);

    /**
     * Add a sample to the buffer
     * @param sample The accelerometer sample to add
     * @return true if added successfully, false if buffer is full
     */
    bool addSample(const AccelSample& sample);

    /**
     * Send buffered samples to New Relic via Particle webhook
     * @return true if publish succeeded, false on error
     */
    bool sendBatch();

    /**
     * Clear the sample buffer
     */
    void clear();

    /**
     * Get number of samples currently buffered
     */
    int getSampleCount() const { return sampleCount; }

    /**
     * Check if buffer is full
     */
    bool isFull() const { return sampleCount >= NR_MAX_SAMPLES; }

    /**
     * Get statistics
     */
    unsigned long getPublishCount() const { return publishCount; }
    unsigned long getFailureCount() const { return failureCount; }
    unsigned long getSamplesDropped() const { return samplesDropped; }

private:
    // Configuration
    char deviceId[32];

    // Sample buffer (static allocation)
    AccelSample samples[NR_MAX_SAMPLES];
    int sampleCount;

    // Statistics
    unsigned long publishCount;
    unsigned long failureCount;
    unsigned long samplesDropped;

    // Helper methods
    bool buildJsonPayload(char* buffer, int bufferSize);
    int appendSampleJson(char* buffer, int offset, int maxSize, const AccelSample& sample, bool isLast);
};

#endif // NEW_RELIC_CLIENT_H
