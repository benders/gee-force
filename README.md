# Gee-Force

A model rocket launch g-force tracker using a Particle Photon microcontroller.

## Hardware

- **Microcontroller**: Particle Photon
- **Display**: Grove 4-digit display
  - CLK: Pin D2
  - DIO: Pin D3
- **Accelerometer**: Grove 3-axis digital accelerometer (MMA7660)
  - Interface: I2C
  - Note: This accelerometer is for prototyping only due to limited range. A higher-range accelerometer will be used in the final version.

## Features

- Real-time g-force measurement and display
- Tracks maximum g-force experienced during flight
- Data streaming to New Relic for analysis and visualization
- High-resolution sampling (20Hz) with batched transmission

## New Relic Integration

The device streams accelerometer data to New Relic as custom events for real-time monitoring and historical analysis.

### Data Format

Each sample is sent as a custom event with:
- `eventType`: `AccelSample`
- `accelX`, `accelY`, `accelZ`: Acceleration in g-forces for each axis
- `magnitude`: Combined magnitude (sqrt of sum of squares)
- `deviceId`: Unique identifier for the device
- `timestamp`: Sample timestamp

### NRQL Queries for Dashboards

#### Chart 1: Three-Axis Acceleration
Shows acceleration across all three axes over time:

```sql
SELECT average(accelX) as 'X-axis',
       average(accelY) as 'Y-axis',
       average(accelZ) as 'Z-axis'
FROM AccelSample
WHERE deviceId = 'YOUR_DEVICE_ID'
TIMESERIES AUTO
SINCE 1 hour ago
```

For detailed view with min/max ranges:
```sql
SELECT average(accelX) as 'X (avg)',
       min(accelX) as 'X (min)',
       max(accelX) as 'X (max)',
       average(accelY) as 'Y (avg)',
       min(accelY) as 'Y (min)',
       max(accelY) as 'Y (max)',
       average(accelZ) as 'Z (avg)',
       min(accelZ) as 'Z (min)',
       max(accelZ) as 'Z (max)'
FROM AccelSample
WHERE deviceId = 'YOUR_DEVICE_ID'
TIMESERIES AUTO
SINCE 1 hour ago
```

#### Chart 2: G-Force Magnitude
Shows combined g-force magnitude over time:

```sql
SELECT average(magnitude) as 'G-Force',
       max(magnitude) as 'Peak G-Force'
FROM AccelSample
WHERE deviceId = 'YOUR_DEVICE_ID'
TIMESERIES AUTO
SINCE 1 hour ago
```

For percentile analysis (useful for detecting spikes):
```sql
SELECT average(magnitude) as 'Average',
       percentile(magnitude, 50) as 'Median',
       percentile(magnitude, 95) as '95th Percentile',
       percentile(magnitude, 99) as '99th Percentile',
       max(magnitude) as 'Peak'
FROM AccelSample
WHERE deviceId = 'YOUR_DEVICE_ID'
TIMESERIES AUTO
SINCE 1 hour ago
```

#### Additional Queries

Find peak g-force events:
```sql
SELECT max(magnitude) as 'Peak G-Force',
       timestamp
FROM AccelSample
WHERE deviceId = 'YOUR_DEVICE_ID'
FACET timestamp
SINCE 1 day ago
LIMIT 10
```

Sample rate verification:
```sql
SELECT count(*) as 'Samples Received'
FROM AccelSample
WHERE deviceId = 'YOUR_DEVICE_ID'
TIMESERIES 1 minute
SINCE 10 minutes ago
```

## Configuration

Before deploying the code, you need to configure your New Relic credentials:

1. **Get your New Relic Insert API Key**:
   - Log in to your New Relic account
   - Go to: [Account settings](https://one.newrelic.com) → API Keys
   - Create or copy an existing "Insert" type API key

2. **Get your New Relic Account ID**:
   - In New Relic, your account ID is visible in the URL: `https://one.newrelic.com/accounts/{ACCOUNT_ID}/...`
   - Or find it in Account Settings

3. **Update the code**:
   - Open `src/gee-force.ino`
   - Replace `YOUR_ACCOUNT_ID` with your account ID
   - Replace `YOUR_INSERT_API_KEY` with your Insert API key

```cpp
#define NR_ACCOUNT_ID "YOUR_ACCOUNT_ID"
#define NR_INSERT_KEY "YOUR_INSERT_API_KEY"
```

## Getting Started

See [CLAUDE.md](CLAUDE.md) for development commands and architecture details.

### Quick Start

```bash
# Install Particle CLI
npm install -g particle-cli

# Log in to Particle
particle login

# Compile and flash to your device
particle flash YOUR_DEVICE_NAME

# Monitor output
particle serial monitor
```

## Project Status

This project is in active development. New Relic integration is currently being implemented.
