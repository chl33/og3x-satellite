# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.6.2] - 2026-03-28

### Added
- **Full Sensor Persistence**: Expanded `saveAll()` and `loadAll()` to include complete sensor metadata (names, units, device classes, and precision). Known devices are now fully recreation-ready on boot, allowing immediate parsing of data packets without waiting for a description packet.
- **Hardware & Software Versioning**: Added version tracking for satellite devices, allowing the bridge to monitor and display firmware levels.
- **Enhanced Telemetry**: Added `packet_count()`, `rssi()`, and `last_packet_millis()` getters to support detailed bridge dashboards.
- **Native Test Support**: Fully modernized the `platformio.ini` and library structure to support native unit testing (`pio test -e native`) in both local and CI environments.

### Changed
- **Mutable Metadata**: Refactored the `Device` class to allow updating names and manufacturer IDs dynamically as new information arrives from LoRa packets.
- **Improved Logging**: Added descriptive logging to the persistence layer to track the success of save/load operations and diagnose JSON parsing issues.

## [0.6.1] - 2026-03-24

### Added
- **Device Persistence**: Implemented static `saveAll()` and `loadAll()` methods in the `Device` class. Bridge applications can now "remember" their satellite network across reboots via a flash-based `devices.json` file.
- **Version Tracking**: Added support for storing and retrieving hardware and software versions for each satellite device.
- **Online Status**: Added `is_online()` getter to the `Device` class to support bridge UI indicators.

### Changed
- **Metadata Mutability**: Refactored the `Device` class to allow updating names, manufacturer IDs, and device types after initial discovery.
- **og3 v0.6.1 Compatibility**: Updated to utilize the new `ConfigInterface::read_file/write_file` methods for robust persistence.

## [0.6.0] - 2026-03-24

### Added
- **API Access**: Added `id_to_float_sensor()` and `id_to_int_sensor()` getters to the `Device` class to support JSON API integrations.

### Changed
- **og3 v0.6.0 Compatibility**: Updated to strictly depend on `og3` 0.6.0 and its modernized dependency management system.

## [0.1.5] - 2026-03-07

### Added
- Initial implementation of the satellite/base-station communication protocol.
