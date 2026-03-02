/**
 * NewRelicClient - Lightweight HTTP client for New Relic Event API
 *
 * Sends batched accelerometer samples to New Relic for analysis and visualization.
 * Designed for memory-constrained microcontrollers with minimal dependencies.
 *
 * Features:
 * - Static memory allocation (no heap fragmentation)
 * - Batched transmission (reduces HTTP overhead)
 * - Simple JSON serialization (no external library needed)
 * - Error handling and connection management
 */

#ifndef NEW_RELIC_CLIENT_H
#define NEW_RELIC_CLIENT_H

#include "Particle.h"

// Configuration
#define NR_MAX_SAMPLES 20              // Max samples per batch (1 second at 20Hz)
#define NR_HOSTNAME "insights-collector.newrelic.com"
#define NR_PORT 443                    // HTTPS port
#define NR_TIMEOUT 5000                // HTTP timeout in milliseconds

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
     * @param accountId New Relic account ID
     * @param insertKey New Relic Insert API key
     * @param deviceId Unique device identifier
     */
    NewRelicClient(const char* accountId, const char* insertKey, const char* deviceId);

    /**
     * Add a sample to the buffer
     * @param sample The accelerometer sample to add
     * @return true if added successfully, false if buffer is full
     */
    bool addSample(const AccelSample& sample);

    /**
     * Send buffered samples to New Relic
     * @return true if transmission succeeded, false on error
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
    unsigned long getSuccessCount() const { return successCount; }
    unsigned long getFailureCount() const { return failureCount; }
    unsigned long getSamplesDropped() const { return samplesDropped; }

private:
    // Configuration
    char accountId[16];
    char insertKey[48];
    char deviceId[32];

    // Sample buffer (static allocation)
    AccelSample samples[NR_MAX_SAMPLES];
    int sampleCount;

    // Statistics
    unsigned long successCount;
    unsigned long failureCount;
    unsigned long samplesDropped;

    // HTTP client
    TCPClient client;

    // Helper methods
    bool buildJsonPayload(char* buffer, int bufferSize);
    int appendSampleJson(char* buffer, int offset, int maxSize, const AccelSample& sample, bool isLast);
    bool sendHttpPost(const char* payload, int payloadLength);
};

#endif // NEW_RELIC_CLIENT_H
