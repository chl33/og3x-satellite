// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/base-station.h>

namespace og3::base_station {
namespace {
// Right now there is only one og3x-satellite device "manufacturer": the author in is basement.
// Maybe someday there will be another?
constexpr uint32_t kC133Org = 0xc133;

bool is_legal(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_') ||
         (c == '-');
}

void make_legal(char* name, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (!is_legal(name[i])) {
      name[i] = '_';
    }
  }
}

std::string legalize(const char* name) {
  std::string lname(name);
  make_legal(&lname[0], lname.length());
  return lname;
}

void make_entry_name(char* entry_name, size_t entry_name_size, const char* device,
                     const char* name) {
  const int len = snprintf(entry_name, entry_name_size, "%s_%s", device, name);
  make_legal(entry_name, len);
}

std::string _manufacturer(uint32_t id) {
  switch (id) {
    case kC133Org:
      return "c133 org";
    default:
      break;
  }

  char buffer[32];
  snprintf(buffer, sizeof(buffer), "manufacturer_%04x", id);
  return buffer;
}

std::string _device_id(const char* name, uint32_t device_id) {
  char buffer[80];
  const auto len = snprintf(buffer, sizeof(buffer), "%s_%x", name, device_id);
  return std::string(buffer, len);
}

}  // namespace

Sensor::Sensor(const char* name, const char* device_class, const char* units, Device* device,
               og3_Sensor_StateClass state_class)
    : m_name(legalize(name)),
      m_device_class(device_class ? device_class : ""),
      m_units(units),
      m_description(name),
      m_state_class(state_class),
      m_device(device) {}

void Sensor::addHAEntry(HADiscovery::Entry& entry) {
  entry.device_name = m_device->cname();
  entry.device_id = m_device->cdevice_id();
  entry.manufacturer = m_device->manufacturer().c_str();
  char entry_name[80];
  make_entry_name(entry_name, sizeof(entry_name), m_device->cname(), name().c_str());
  entry.entry_name = entry_name;
  switch (m_state_class) {
    case og3_Sensor_StateClass_STATE_CLASS_UNSPECIFIED:
      break;
    case og3_Sensor_StateClass_STATE_CLASS_MEASUREMENT:
      entry.state_class = "measurement";
      break;
  }
  // entry.software
  // entry.model
  // entry.icon
  JsonDocument json;
  // TODO(chrishl): should bookkeep and send again if this fails.
  m_device->ha_discovery().addEntry(&json, entry);
}

FloatSensor::FloatSensor(const char* name, const char* device_class, const char* units,
                         unsigned decimals, Device* device, og3_Sensor_StateClass state_class)
    : Sensor(name, device_class, units, device, state_class),
      m_value(m_name.c_str(), 0.0f, m_units.c_str(), "", 0, decimals, device->vg()) {
  HADiscovery::Entry entry(m_value, ha::device_type::kSensor, m_device_class.c_str());
  addHAEntry(entry);
}

IntSensor::IntSensor(const char* name, const char* device_class, const char* units, Device* device,
                     og3_Sensor_StateClass state_class)
    : Sensor(name, device_class, units, device, state_class),
      m_value(m_name.c_str(), 0, m_units.c_str(), "", 0, device->vg()) {
  HADiscovery::Entry entry(m_value, ha::device_type::kSensor, m_device_class.c_str());
  addHAEntry(entry);
}

Device::Device(uint32_t device_id_num, const char* name, uint32_t mfg_id, const char* device_type,
               ModuleSystem* module_system, HADiscovery* ha_discovery, uint16_t seq_id,
               VariableGroup& cvg)
    : m_device_id_num(device_id_num),
      m_name(name),
      m_device_id(_device_id(name, device_id_num)),
      m_manufacturer(_manufacturer(mfg_id)),
      m_device_type(device_type),
      m_seq_id(seq_id),
      m_discovery(ha_discovery),
      m_vg(m_name.c_str(), m_device_id.c_str()),
      m_dropped_packets("dropped_packets", 0, "count", "dropped packets", 0, m_vg),
      m_rssi("RSSI", 0, "dB", "", 0, m_vg),
      m_str_disabled(m_name + "_disabled"),
      m_disabled(m_str_disabled.c_str(), false, nullptr, VariableBase::kSettable, cvg) {
  JsonDocument json;
  auto make_ha_entry = [&json, this](const VariableBase& var, const char* device_type,
                                     const char* device_class) {
    json.clear();
    HADiscovery::Entry entry(var, device_type, device_class);
    entry.device_name = cname();
    entry.device_id = cdevice_id();
    entry.manufacturer = manufacturer().c_str();
    if (!m_device_type.empty()) {
      entry.model = cdevice_type();
    }
    char entry_name[80];
    make_entry_name(entry_name, sizeof(entry_name), cname(), var.name());
    entry.entry_name = entry_name;
    m_discovery->addEntry(&json, entry);
  };
  make_ha_entry(m_dropped_packets, ha::device_type::kSensor, nullptr);
  make_ha_entry(m_rssi, ha::device_type::kSensor, ha::device_class::sensor::kSignalStrength);
}

void Device::got_packet(uint16_t seq_id, int rssi) {
  if (seq_id > m_seq_id) {
    m_dropped_packets = m_dropped_packets.value() + static_cast<int>(seq_id) - 1 - m_seq_id;
  } else if (seq_id == m_seq_id) {
    // Not sure why this would happen (maybe not setting seq-id in the sender), so ignore it.
  } else {
    const uint16_t diff = seq_id - 1 - m_seq_id;  // difference with wrapping
    if (diff < 256) {
      m_dropped_packets = m_dropped_packets.value() + diff;
    } else {
      // Assume that the remote device reset, so sent seq_id started with 0.
      m_dropped_packets = m_dropped_packets.value() + seq_id;
    }
  }
  m_seq_id = seq_id;
  m_rssi = rssi;
}

}  // namespace og3::base_station
