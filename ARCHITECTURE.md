# Gee-Force Architecture

## Overview

```
+------------------+        TCP:1883        +-------------------------+
|  Particle Photon |  ---MQTT (QoS 0)-->   |   nr-mqtt (Docker host) |
|                  |                        |                         |
|  MMA7660 (I2C)   |                        |  +---------+            |
|  Grove Display   |                        |  |Mosquitto|            |
|  hirotakaster/   |                        |  | broker  |            |
|  MQTT library    |                        |  +---------+            |
+------------------+                        |       |                 |
                                            |  +---------+   HTTPS    |
                                            |  |  relay  |---------> NR
                                            |  | (Node)  |  Event API |
                                            |  +---------+            |
                                            +-------------------------+
```

The device connects directly to the Mosquitto broker on the local LAN. The relay
service runs alongside the broker in Docker Compose, subscribes to all geeforce
topics, batches samples, and forwards them to the New Relic Event API over HTTPS.

---

## MQTT Topic Structure

| Topic                          | Direction      | Purpose                        |
|-------------------------------|----------------|--------------------------------|
| `geeforce/{deviceId}/accel`   | device → broker | Accelerometer sample (1/msg)  |
| `geeforce/{deviceId}/status`  | device → broker | Heartbeat / connection status |

`{deviceId}` is the Particle device ID string (24-char hex), e.g.
`geeforce/3b003f000747343232363230/accel`.

The relay subscribes to `geeforce/+/accel` to receive samples from all devices.

---

## Message Format

### Accel sample (`geeforce/{deviceId}/accel`)

Single sample per message. Short field names reduce payload size on the
constrained Photon (~120 KB RAM, static 128-byte MQTT payload buffer).

```json
{"ts":1740000000123,"x":0.123,"y":-0.456,"z":9.789,"m":9.803,"dev":"3b003f000747343232363230"}
```

| Field | Type   | Description                                  |
|-------|--------|----------------------------------------------|
| `ts`  | uint64 | Unix timestamp in milliseconds (epoch ms)    |
| `x`   | float  | X-axis acceleration in g (3 decimal places)  |
| `y`   | float  | Y-axis acceleration in g (3 decimal places)  |
| `z`   | float  | Z-axis acceleration in g (3 decimal places)  |
| `m`   | float  | Magnitude sqrt(x²+y²+z²) in g               |
| `dev` | string | Particle device ID                           |

Approximate serialized size: ~90 bytes. Well within the 128-byte static buffer.

### Status heartbeat (`geeforce/{deviceId}/status`)

```json
{"state":"online","dev":"3b003f000747343232363230","ts":1740000000000}
```

Published at connection and periodically (recommended: every 30s). The relay
does not forward status messages to New Relic; they are for broker-side
diagnostics only.

### New Relic event schema

The relay maps each MQTT message to a New Relic custom event:

```json
{
  "eventType": "AccelSample",
  "timestamp":  1740000000123,
  "accelX":     0.123,
  "accelY":    -0.456,
  "accelZ":     9.789,
  "magnitude":  9.803,
  "deviceId":  "3b003f000747343232363230"
}
```

Field names are expanded to their full form when forwarding to New Relic so
existing NRQL queries and dashboards remain compatible.

---

## Security Model

### Broker authentication

Mosquitto is configured with `allow_anonymous false` and a `password_file`.
Every connecting client must supply a username and password. The device
credential is a dedicated low-privilege account (no admin rights).

### Device credentials

The MQTT username and password are compiled into the firmware as preprocessor
defines. They must never be committed to the repository.

**Pattern — credentials header file (gitignored):**

Create `src/credentials.h` (listed in `.gitignore`):

```cpp
// src/credentials.h  — DO NOT COMMIT
#define MQTT_BROKER_HOST  "192.168.1.50"
#define MQTT_BROKER_PORT  1883
#define MQTT_USERNAME     "geeforce-device"
#define MQTT_PASSWORD     "changeme"
```

Include it from the firmware:

```cpp
#include "credentials.h"
```

A template is checked into the repository as `src/credentials.h.example` with
placeholder values. Developers copy it to `src/credentials.h` and fill in real
values before building.

`src/credentials.h` is listed in `.gitignore` and `.particleignore`.

### New Relic API key

The New Relic Insert Key lives only in the `.env` file on the Docker host. It
is never sent to the device and never appears in firmware or MQTT messages.

### TLS limitation

The `hirotakaster/MQTT` Particle library (the only maintained MQTT library for
Device OS) does **not** support TLS. The connection between the device and
broker is plain TCP on port 1883.

**Mitigations:**

1. Deploy the broker on the same LAN segment as the device. Do not expose
   port 1883 to the internet.
2. Use firewall rules (iptables / router ACL) to restrict port 1883 to the
   device's IP or MAC address.
3. For production deployments outside a trusted LAN, establish a WireGuard or
   OpenVPN tunnel between the device network and the broker host. The MQTT
   connection itself remains plain TCP but travels inside the encrypted tunnel.

The relay-to-New Relic leg is always HTTPS (TLS 1.2+), so credentials and data
are protected on the internet path.

---

## Connection Model

- The broker IP or hostname is compiled into the firmware via `MQTT_BROKER_HOST`
  in `src/credentials.h`.
- The device connects to `MQTT_BROKER_HOST:1883` over TCP immediately after
  WiFi association.
- The broker should be on the same LAN as the device. If the broker moves,
  reflash the firmware with the new address, or store the address in EEPROM and
  update it over-the-air via a Particle variable/function.
- MQTT client ID is set to the Particle device ID to guarantee uniqueness per
  broker.

---

## Reconnection Strategy

The firmware uses exponential backoff when the MQTT connection is lost:

```
attempt 1:  wait  2s
attempt 2:  wait  4s
attempt 3:  wait  8s
attempt 4:  wait 16s
attempt 5:  wait 32s
attempt 6+: wait 60s  (cap)
```

The watchdog timer (60s) is petted during the backoff wait loop so the device
does not reset while reconnecting. Samples collected while disconnected are
dropped (buffer overflow) — acceptable for sensor telemetry; no persistent
store is available on the Photon.

---

## Data Flow (detailed)

```
Photon loop()
  |
  +-- every 333ms: read MMA7660 via I2C
  |     accelX, accelY, accelZ, magnitude, timestamp
  |
  +-- immediately: serialize to JSON (snprintf into 128-byte static buffer)
  |     {"ts":...,"x":...,"y":...,"z":...,"m":...,"dev":"..."}
  |
  +-- MQTT publish (QoS 0, retain=false)
  |     topic: geeforce/{deviceId}/accel
  |
  v
Mosquitto broker (port 1883)
  |
  +-- authenticates client (username/password)
  +-- routes message to subscribers
  |
  v
Relay service (Node.js, same Docker network)
  |
  +-- receives message on geeforce/+/accel
  +-- parses JSON, maps short fields to full NR field names
  +-- appends to in-memory batch array
  |
  +-- flush condition (whichever comes first):
  |     - batch size reaches 500 events
  |     - 2-second timer fires
  |
  +-- POST to NR Event API (HTTPS)
  |     URL:  https://insights-collector.newrelic.com/v1/accounts/{accountId}/events
  |     Auth: X-Insert-Key: {NEW_RELIC_INSERT_KEY}
  |     Body: JSON array of AccelSample events
  |
  v
New Relic NRDB (queryable via NRQL)
```

---

## QoS

QoS 0 (fire and forget) is used for all publishes. Rationale:

- Sensor telemetry is loss-tolerant; a missed sample is acceptable.
- QoS 1/2 requires the broker to store messages and the client to maintain a
  session, which increases RAM usage and firmware complexity on the Photon.
- At 3 Hz the data rate is low; occasional packet loss has negligible impact on
  the time-series data quality in New Relic.

---

## New Relic Batching

The relay accumulates events in memory and flushes when:

- The batch contains **500 events**, OR
- **2 seconds** have elapsed since the last flush (whichever comes first).

At 3 Hz per device the 2-second window produces ~6 events per flush, well
within the NR Event API limit of 1 MB per POST. The 500-event cap prevents
unbounded memory growth if the flush timer is delayed.

Failed POST requests are retried up to 3 times with a 1-second delay between
attempts. After 3 failures the batch is discarded and an error is logged.
