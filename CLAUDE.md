# CLAUDE.md

## Project Overview

Gee-Force tracks g-forces during model rocket launches using a Particle Photon. It reads acceleration via MMA7660 (I2C), displays current g-force on a Grove 4-digit display, and streams data to New Relic via MQTT.

**Status**: Prototyping with MMA7660 (~1.5-3g range). Final version needs high-g accelerometer (e.g., ADXL377 for ±200g).

## Development Commands

```bash
# Setup
particle login
particle list

# Build & flash
# IMPORTANT: target Device OS 2.3.1 — MQTT v0.4.32 is incompatible with 3.x
particle compile photon . --target 2.3.1
particle flash <device_name> --target 2.3.1 <firmware.bin>
particle flash --usb <firmware.bin>  # USB, device in DFU mode

# Monitor
particle serial monitor
particle subscribe geeforce           # watch MQTT-related Particle events (debug)

# Libraries
particle library add <library_name>   # updates project.properties

# MQTT broker + relay (run from ../nr-mqtt/)
cp ../nr-mqtt/.env.example ../nr-mqtt/.env   # first time: fill in NR credentials
docker compose -f ../nr-mqtt/docker-compose.yml up -d
docker compose -f ../nr-mqtt/docker-compose.yml logs -f

# Provision device credential in Mosquitto password file
docker run --rm -v $(pwd)/../nr-mqtt/mosquitto:/mosquitto eclipse-mosquitto:2 \
  mosquitto_passwd -b -c /mosquitto/passwd geeforce-device <password>
docker compose -f ../nr-mqtt/docker-compose.yml kill -s SIGHUP mosquitto

# Watch live MQTT messages from the device
mosquitto_sub -h localhost -p 1883 -u geeforce-device -P <password> \
  -t 'geeforce/#' -v
```

See [ARCHITECTURE.md](ARCHITECTURE.md) for full system design and security model.

## Architecture

**Pins**: D2=CLK, D3=DIO (display); D0=SDA, D1=SCL (I2C accelerometer)

**Data flow**: Accelerometer → 3Hz sample loop → serialize to compact JSON → MQTT publish (QoS 0) → Mosquitto broker (port 1883) → relay service → New Relic Event API (HTTPS)

**MQTT topics**:
- `geeforce/{deviceId}/accel` — one accelerometer sample per message
- `geeforce/{deviceId}/status` — heartbeat

**Message format** (compact, ~90 bytes):
```json
{"ts":1740000000123,"x":0.123,"y":-0.456,"z":9.789,"m":9.803,"dev":"abc123"}
```

**Event schema** (`eventType: AccelSample`): `timestamp`, `accelX`, `accelY`, `accelZ`, `magnitude`, `deviceId`

**MQTT library**: `hirotakaster/MQTT` — plain TCP only (no TLS). Deploy broker on the same LAN as the device. See ARCHITECTURE.md for security mitigations.

**Broker**: Mosquitto running in Docker (`../nr-mqtt/`). Username/password auth required (`allow_anonymous false`).

## Key Patterns

- **Credentials**: MQTT username/password are defined in `src/credentials.h` (gitignored). Copy `src/credentials.h.example` to `src/credentials.h` and fill in values before building. The New Relic Insert Key lives only in `../nr-mqtt/.env` and never touches the device.
- **Static buffers**: Use `snprintf()` with static arrays, not `String::format()` — avoids heap fragmentation.
- **Watchdog**: 60-second `ApplicationWatchdog` with `checkin()` calls; device auto-resets on hang.
- **I2C**: Tested stable at 3Hz. Reduce rate if bus issues occur. Grove modules have built-in pull-ups.
- **Display**: Shows "X.XX" format. Values >9.99g overflow display but are still sampled and published.
- **`SYSTEM_MODE(AUTOMATIC)`**: Device OS manages cloud connectivity automatically.

## Technologies

- **Platform**: Particle Photon (ARM Cortex M3), C++ Arduino-style
- **Device OS**: **2.3.1** (required — MQTT v0.4.32 hard-faults on Device OS 3.x)
- **Libraries**:
  - `Grove_4Digit_Display` v1.0.2
  - `MMA7660-Accelerometer` v0.0.3
  - `hirotakaster/MQTT` v0.4.32 (MQTT client, plain TCP)
