# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
