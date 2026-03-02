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
- Data logging to Particle Cloud (optional)

## Getting Started

See [CLAUDE.md](CLAUDE.md) for development commands and architecture details.

## Project Status

This project is in early prototyping phase.
