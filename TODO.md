# TODO - Gee-Force Project

## Current Sprint: New Relic Integration

### Phase 1: Foundation ✓
- [x] Design NRQL queries for dashboards
- [x] Document New Relic integration architecture
- [x] Update README.md with NRQL examples

### Phase 2: Core Implementation (In Progress)
- [ ] Create New Relic HTTP client library
  - [ ] Design library structure (lib/NewRelicClient)
  - [ ] Implement HTTP POST using TCPClient
  - [ ] Add JSON payload builder (minimal, no external library)
  - [ ] Handle connection lifecycle
  - [ ] Add error handling and retry logic

- [ ] Implement sample buffering
  - [ ] Define AccelSample struct
  - [ ] Create circular buffer (20 samples)
  - [ ] Add buffer management functions (add, clear, isFull)
  - [ ] Implement timestamp tracking

- [ ] Modify main program
  - [ ] Integrate NewRelicClient library
  - [ ] Add sample buffering to loop()
  - [ ] Implement 1-second batch transmission
  - [ ] Add configuration for account ID and API key
  - [ ] Maintain display updates at current rate

### Phase 3: Testing & Validation
- [ ] Local testing
  - [ ] Verify buffer management (no memory leaks)
  - [ ] Test batch JSON generation
  - [ ] Validate timing (samples at 20Hz, transmit at 1Hz)

- [ ] Integration testing
  - [ ] Deploy to device
  - [ ] Monitor serial output for errors
  - [ ] Verify HTTP POST success

- [ ] Data validation
  - [ ] Query New Relic API to confirm data arrival
  - [ ] Verify sample rate (expect ~1200 samples/minute)
  - [ ] Test NRQL queries from README.md
  - [ ] Create test dashboard with both charts

### Phase 4: Production Readiness
- [ ] Performance optimization
  - [ ] Profile memory usage
  - [ ] Optimize JSON serialization
  - [ ] Fine-tune timing to prevent I2C conflicts

- [ ] Error handling
  - [ ] Handle network connectivity loss gracefully
  - [ ] Add sample drop counter for monitoring
  - [ ] Log transmission failures

- [ ] Documentation
  - [ ] Add setup instructions for New Relic API key
  - [ ] Document configuration parameters
  - [ ] Add troubleshooting guide

## Future Enhancements

### Hardware Upgrades
- [ ] Replace MMA7660 with high-g accelerometer (ADXL377 ±200g)
- [ ] Add SD card for local data backup during flight
- [ ] Implement battery voltage monitoring
- [ ] Add GPS module for location tracking

### Software Features
- [ ] Implement offline buffering with sync on reconnect
- [ ] Add configurable sample rate
- [ ] Create web-based configuration portal
- [ ] Implement firmware OTA updates
- [ ] Add launch detection algorithm (automatic event marking)

### Analytics
- [ ] Create pre-built New Relic dashboard
- [ ] Implement anomaly detection alerts
- [ ] Add comparative analysis (multiple flights)
- [ ] Create flight report generator

## Known Issues
None currently.

## Completed Tasks
- Initial prototype with MMA7660 accelerometer
- 4-digit display integration
- Basic g-force calculation and display
- Application watchdog implementation
- I2C bus management optimization
- Published to New Relic GitHub Enterprise
