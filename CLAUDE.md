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

# Subscribe to published events
particle subscribe gforce
```

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
6. Buffer samples in memory (20 samples per second)
7. Every 1 second, batch and send buffered samples to New Relic
8. Repeat

### New Relic Integration

**Event API Details**:
- **Endpoint**: `https://insights-collector.newrelic.com/v1/accounts/{accountId}/events`
- **Authentication**: Insert API Key (X-Insert-Key header)
- **Event Type**: `AccelSample`
- **Data Format**: JSON array of events (batched)

**Batching Strategy**:
- Samples collected at 20Hz (50ms intervals)
- 20 samples buffered per second
- Transmitted as batch every 1 second to minimize HTTP overhead
- Static circular buffer to avoid heap fragmentation
- Memory-efficient: ~80 bytes per sample × 20 samples = 1.6KB buffer

**Event Schema**:
```json
{
  "eventType": "AccelSample",
  "timestamp": 1234567890123,
  "accelX": 0.123,
  "accelY": 0.456,
  "accelZ": 0.789,
  "magnitude": 0.935,
  "deviceId": "photon_12345"
}
```

**HTTP Client**:
- Custom lightweight HTTP client library (lib/NewRelicClient)
- Minimizes 3rd party dependencies
- Uses Particle TCPClient for HTTP POST
- Handles connection management and error recovery
- Non-blocking where possible to maintain sampling rate

### Important Patterns

- **SYSTEM_THREAD(ENABLED)**: CRITICAL for stability. Without this, Particle.publish() calls can block indefinitely if cloud connection is slow, freezing the entire device. With SYSTEM_THREAD enabled, user code runs independently of cloud connectivity.

- **I2C Bus Management**: The Photon's I2C bus can become overloaded with excessive traffic. Display updates are limited to 1Hz (synchronized with cloud publishes) to prevent I2C bus lockup. Running display updates at 10Hz caused device hangs after 10-50 cycles.

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
