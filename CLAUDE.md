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
# Compile the project in the cloud
particle compile photon

# Flash to a specific device over-the-air
particle flash <device_name> src/

# Flash to a device via USB (device must be in DFU mode)
particle flash --usb src/

# Compile and flash in one command
particle flash <device_name>
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
   - Updates display at 10Hz

4. **Cloud Publishing**:
   - Publishes g-force readings to Particle Cloud
   - Event name: "gforce"
   - Can be used for data logging and analysis

### Data Flow

1. Loop runs at 10Hz (100ms delay)
2. Read accelerometer x, y, z values via I2C
3. Convert raw values to g-force units
4. Calculate total g-force magnitude
5. Update maximum g-force if exceeded
6. Display current value on 4-digit display
7. Publish to Particle Cloud for logging
8. Repeat

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
