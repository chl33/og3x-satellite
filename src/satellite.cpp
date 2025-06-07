// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "og3/satellite.h"

#define SETSTR(X, VAL) strncpy(X, (VAL), sizeof(X) - 1)

namespace og3::satellite {

bool PacketFloatReading::write(og3_Packet& packet) {
  auto& reading = packet.reading[packet.reading_count];
  reading.sensor_id = m_sensor_id;
  reading.value = m_var.value();
  packet.reading_count += 1;
  return true;
}

bool PacketFloatReading::write_desc(og3_Packet& packet) {
  auto& sensor = packet.sensor[packet.sensor_count];
  sensor.id = m_sensor_id;
  SETSTR(sensor.name, m_var.name());
  SETSTR(sensor.units, m_var.units());
  sensor.type = m_sensor_type;
  packet.sensor_count += 1;
  return true;
}

bool PacketIntReading::write(og3_Packet& packet) {
  auto& reading = packet.i_reading[packet.i_reading_count];
  reading.sensor_id = m_sensor_id;
  reading.value = m_ivar.value();
  packet.i_reading_count += 1;
  return true;
}

bool PacketIntReading::write_desc(og3_Packet& packet) {
  auto& sensor = packet.sensor[packet.sensor_count];
  sensor.id = m_sensor_id;
  SETSTR(sensor.name, m_desc);
  sensor.type = og3_Sensor_Type_TYPE_INT_NUMBER;
  packet.sensor_count += 1;
  return true;
}

void PacketSender::send_desc() {
  m_app->log().debugf("send_reading_i_with_desc(%u)", m_rtc->sensor_descriptions_sent);
  if (m_rtc->sensor_descriptions_sent >= m_readings.size()) {
    return;
  }
  m_is_sending = true;
  auto reading = m_readings[m_rtc->sensor_descriptions_sent].get();
  if (!reading->read()) {
    return;
  }
  og3_Packet packet og3_Packet_init_zero;
  start_packet(packet, true);
  reading->write_desc(packet);
  m_rtc->sensor_descriptions_sent += 1;
  const bool more_to_send = (m_rtc->sensor_descriptions_sent < m_readings.size());
  // Don't blink if board will go to sleep immediately after sending the packet.
  send_packet(packet);
  m_is_sending = more_to_send;
  if (more_to_send) {
    m_app->tasks().runIn(15 * kMsecInSec, [this]() { send_desc(); });
  }
}

void PacketSender::send_all_readings() {
  for (auto& fr : m_readings) {
    fr->read();
  }
  m_app->log().debug("PacketSender::update() preparing packet.");
  og3_Packet packet og3_Packet_init_zero;
  // When re-sending descriptions, also include device description with first packet.
  const bool update_device = (m_rtc->sensor_descriptions_sent == 0);
  start_packet(packet, update_device);
  for (size_t i = 0; i < m_readings.size(); i++) {
    m_readings[i]->write(packet);
    if (m_rtc->sensor_descriptions_sent == i) {
      m_readings[i]->write_desc(packet);
    }
  }
  send_packet(packet);
  if (m_rtc->sensor_descriptions_sent < m_readings.size()) {
    m_rtc->sensor_descriptions_sent += 1;
  }
}

void PacketSender::start_packet(og3_Packet& packet, bool update_device) {
  packet.device_id = m_board_id;
  packet.has_device = update_device;
  if (!update_device) {
    return;
  }
  packet.device.id = m_board_id;
  packet.device.manufacturer = m_device->manufacturer;
  SETSTR(packet.device.name, m_device->name);

  packet.device.hardware_version.major = m_device->hardware_version.major;
  packet.device.hardware_version.minor = m_device->hardware_version.minor;
  packet.device.software_version.major = m_device->software_version.major;
  packet.device.software_version.minor = m_device->software_version.minor;
  packet.device.software_version.patch = m_device->software_version.patch;
}

}  // namespace og3::satellite
