# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Gee-Force is an IoT project for tracking g-forces during model rocket launches using a Particle Photon microcontroller. The device measures acceleration in real-time, displays the current g-force on a 4-digit display, and tracks the maximum g-force experienced during flight.

**Current Status**: Prototyping phase with MMA7660 accelerometer (limited range). Final version will use a higher-range accelerometer suitable for rocket launches.

## Development Commands

### Setup
```bash
# Install Particle CLI (if not already installed)
npm install -g particle-cli

# Log in to your Particle account
particle login

# List your devices
particle list
```

### Building and Flashing
```bash
# Check status and find device name
particle list

# Compile the project in the cloud
particle compile photon

# Compile and flash over the cloud in one command
particle flash <device_name>

# Flash file to a device via USB (device must be in DFU mode)
particle flash --usb <firmware-file.bin>
```

### Monitoring
```bash
# Monitor serial output from the device
particle serial monitor

# Monitor device logs
particle monitor <device_name>

# Subscribe to published events (New Relic data)
particle subscribe nr_accel

# Monitor webhook execution
# View in Particle Console: Integrations → nr_accel → View Logs
```

### Webhook Setup
```bash
# Create webhook from JSON file
particle webhook create nr-webhook.json

# List webhooks
particle webhook list

# Delete webhook
particle webhook delete nr_accel
```

See [WEBHOOK_SETUP.md](WEBHOOK_SETUP.md) for detailed webhook configuration instructions.

### Library Management
```bash
# List available libraries
particle library list

# Search for a library
particle library search <name>

# Add a library to the project (this updates project.properties)
particle library add <library_name>
```

## Architecture

### Project Structure
```
gee-force/
├── src/
│   └── gee-force.ino    # Main application code
├── lib/                  # Local library files (if any)
├── project.properties    # Library dependencies
├── CLAUDE.md            # This file
├── TODO.md              # Project tasks
└── README.md            # Project documentation
```

### Hardware Configuration

**Particle Photon Pinout**:
- **D2**: CLK pin for Grove 4-digit display
- **D3**: DIO pin for Grove 4-digit display
- **I2C**: Used by MMA7660 accelerometer (default Photon I2C pins: D0=SDA, D1=SCL)

### Key Components

1. **Accelerometer Interface** (`MMA7660`):
   - Reads 3-axis acceleration data via I2C
   - Converts raw values to g-force units (1/21.3 g per LSB)
   - Provides x, y, z acceleration components

2. **Display Controller** (`TM1637`):
   - Controls Grove 4-digit 7-segment display
   - Shows current g-force with decimal point
   - Format: "X.XX" (e.g., "1.23" for 1.23g)

3. **G-Force Calculation**:
   - Computes total magnitude: sqrt(ax² + ay² + az²)
   - Tracks maximum g-force throughout session
   - Updates display at 20Hz

4. **Sample Buffer**:
   - Circular buffer storing 20 samples (1 second of data)
   - Each sample: timestamp, x, y, z, magnitude
   - Static allocation to avoid heap fragmentation
   - Automatically wraps at buffer end

5. **New Relic Client** (`lib/NewRelicClient`):
   - Lightweight HTTP client for Event API
   - Batches samples into JSON payload
   - Handles connection lifecycle
   - Error recovery and retry logic
   - Non-blocking transmission where possible

### Data Flow

Current implementation:
1. Loop runs at 20Hz (50ms sample interval)
2. Read accelerometer x, y, z values via I2C
3. Convert raw values to g-force units
4. Calculate total g-force magnitude
5. Update 4-digit display with current magnitude
6. Buffer samples in memory (5 samples per batch)
7. Every 250ms, batch and publish to Particle Cloud
8. Particle webhook forwards to New Relic Event API
9. Repeat

Full path: Accelerometer → Device → Particle Cloud → Webhook → New Relic → NRQL Queries → Dashboards

### New Relic Integration

**Architecture: Webhook-Based (Secure)**:
- Device publishes events to Particle Cloud using `Particle.publish()`
- Particle webhook forwards to New Relic Event API with credentials injected
- Credentials never stored on device or in source code
- HTTPS handled by Particle Cloud (Photon has limited TLS support)

**Event Flow**:
```
Device → Particle.publish() → Particle Cloud → Webhook → New Relic Event API
```

**Webhook Configuration**:
- **Event Name**: `nr_accel`
- **Endpoint**: `https://insights-collector.newrelic.com/v1/accounts/{accountId}/events`
- **Authentication**: X-Insert-Key header (configured in webhook)
- **Method**: POST with JSON payload

**Batching Strategy**:
- Samples collected at 20Hz (50ms intervals)
- 5 samples buffered per batch
- Transmitted every 250ms (4 batches/second)
- Limited by Particle.publish() constraints:
  - Max payload: 622 bytes
  - Max rate: 4 events/second sustained
- Static circular buffer to avoid heap fragmentation
- Memory-efficient: ~120 bytes per sample × 5 samples = 600 bytes

**Event Schema**:
```json
[
  {
    "eventType": "AccelSample",
    "timestamp": 1234567890123,
    "accelX": 0.123,
    "accelY": 0.456,
    "accelZ": 0.789,
    "magnitude": 0.935,
    "deviceId": "photon_12345"
  },
  ...
]
```

**NewRelicClient Library** (`lib/NewRelicClient`):
- Lightweight batching and JSON serialization
- Minimizes 3rd party dependencies
- Uses Particle.publish() for transmission
- Handles buffer management and statistics
- No HTTP/TLS code on device (handled by webhook)

### Important Patterns

- **SYSTEM_MODE(AUTOMATIC)**: Lets Particle Device OS manage cloud connectivity automatically. The device will attempt to stay connected to the Particle Cloud for reliable webhook delivery.

- **Particle Webhooks for Credentials**: CRITICAL security pattern. Never hardcode API keys or credentials in device firmware. Use webhooks to keep credentials in the cloud where they can be updated without reflashing devices.

- **I2C Bus Management**: The Photon's I2C bus can become overloaded with excessive traffic. The current implementation samples at 20Hz which has been tested stable. If issues occur, reduce sampling rate.

- **Particle.publish() Rate Limits**: Critical constraint for webhook-based telemetry:
  - Burst limit: 1 event/second
  - Sustained limit: 4 events/second (averaged over 1 minute)
  - Payload limit: 622 bytes per event
  - Current config: 4 batches/second at ~600 bytes each (at sustained limit)
  - If rate limiting occurs, reduce TRANSMIT_INTERVAL or NR_MAX_SAMPLES

- **Application Watchdog**: A 60-second watchdog timer with ApplicationWatchdog::checkin() calls prevents permanent hangs. Device will auto-reset if code freezes.

- **Static Buffers**: Use snprintf() with static char arrays instead of String::format() to avoid heap fragmentation on resource-constrained devices.

- **MMA7660 Limitations**: The current accelerometer has a limited range (~1.5g to 3g max depending on configuration). This is intentional for prototyping. When testing with actual rocket launches, replace with a high-g accelerometer (e.g., ADXL377 for ±200g).

- **Display Format**: The 4-digit display shows values with 2 decimal places. Values above 9.99g will overflow the display but will still be tracked in memory and published to the cloud.

- **I2C Communication**: The MMA7660 uses I2C. Ensure proper pull-up resistors are present on the I2C lines (usually built into Grove modules).

## Technologies

- **Platform**: Particle Photon (ARM Cortex M3 microcontroller)
- **Language**: C++ (Arduino-style framework)
- **Build System**: Particle Cloud Compiler
- **Libraries** (verified working with Device OS 3.3.1):
  - `Grove_4Digit_Display` v1.0.2: TM1637 display driver
  - `MMA7660-Accelerometer` v0.0.3: MMA7660 accelerometer driver

## Configuration

- **project.properties**: Declares library dependencies with versions
- **Particle Device OS**: Ensure device is running compatible firmware version
- **Particle Cloud**: Used for compilation, flashing, and data publishing

## Hardware Notes

- The Grove modules use 3.3V logic, compatible with Particle Photon
- Ensure stable power supply during high-g events
- Consider mounting orientation: current code calculates total magnitude, not directional g-force
- For final deployment, consider adding data storage (SD card) in case of connectivity loss during flight
