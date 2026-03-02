# TODO

Project task list for Gee-Force rocket g-force tracker.

## Prototyping Phase (Current)

### Hardware Setup
- [ ] Wire Grove 4-digit display to Photon (D2=CLK, D3=DIO)
- [ ] Wire Grove MMA7660 accelerometer to Photon (I2C)
- [ ] Test display with simple patterns
- [ ] Test accelerometer readings at rest (should read ~1g)
- [ ] Verify I2C communication is stable

### Software Development
- [ ] Test basic accelerometer readings via serial monitor
- [ ] Calibrate display to show g-force correctly
- [ ] Test g-force calculation accuracy
- [ ] Implement zero-point calibration (subtract gravity at rest)
- [ ] Add button to reset max g-force reading
- [ ] Test Particle Cloud publishing
- [ ] Add error handling for sensor initialization failures

### Testing & Validation
- [ ] Test with hand movements (gentle shaking)
- [ ] Verify display updates at correct rate
- [ ] Check that max g-force tracking works correctly
- [ ] Test data logging to Particle Cloud
- [ ] Validate accuracy against known accelerations

## Production Phase (Future)

### Hardware Upgrades
- [ ] Research high-g accelerometers (ADXL377 ±200g or similar)
- [ ] Order production accelerometer
- [ ] Design mounting solution for rocket
- [ ] Consider adding SD card for offline data storage
- [ ] Add power management (battery level monitoring)
- [ ] Design protective enclosure

### Software Enhancements
- [ ] Update code for new accelerometer
- [ ] Implement data buffering for SD card storage
- [ ] Add launch detection algorithm (detect liftoff automatically)
- [ ] Implement peak detection and event marking
- [ ] Add post-flight data export functionality
- [ ] Optimize power consumption for battery operation

### Flight Testing
- [ ] Bench test with high-g accelerometer
- [ ] Conduct low-power rocket test flights
- [ ] Validate data accuracy against simulation
- [ ] Test recovery and data retrieval
- [ ] Conduct final test flight before competition

## Documentation
- [ ] Document wiring diagrams
- [ ] Create user manual for operation
- [ ] Document data format and analysis procedures
- [ ] Add photos of assembly
- [ ] Create troubleshooting guide

## Nice-to-Have Features
- [ ] Add WiFi web interface for configuration
- [ ] Real-time graphing via web dashboard
- [ ] Multiple sensor fusion (gyroscope + accelerometer)
- [ ] Audio feedback (beeps for milestones)
- [ ] LED status indicators
