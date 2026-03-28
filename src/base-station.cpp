// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/base-station.h>
#include <og3/config_interface.h>

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
  switch (m_state_class) {
    case og3_Sensor_StateClass_STATE_CLASS_UNSPECIFIED:
      break;
    case og3_Sensor_StateClass_STATE_CLASS_MEASUREMENT:
      entry.state_class = "measurement";
      break;
  }
  m_device->addHAEntry(entry, name().c_str());
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
      m_mfg_id(mfg_id),
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
    HADiscovery::Entry entry(var, device_type, device_class);
    entry.state_class = "measurement";
    addHAEntry(entry, var.name());
  };
  make_ha_entry(m_dropped_packets, ha::device_type::kSensor, nullptr);
  make_ha_entry(m_rssi, ha::device_type::kSensor, ha::device_class::sensor::kSignalStrength);
  setIsOnline(true);
}

bool Device::saveAll(const char* filename, ConfigInterface* config,
                     const std::map<uint32_t, std::unique_ptr<Device>>& devices) {
  if (!config) {
    return false;
  }
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (auto& iter : devices) {
    const auto& device = iter.second;
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = device->id_num();
    obj["name"] = device->name();
    obj["mfg"] = device->mfg_id();
    obj["type"] = device->device_type();
    obj["timeout"] = device->comms_timeout_millis();
  }
  String content;
  serializeJson(doc, content);
  return config->write_file(filename, content.c_str());
}

bool Device::loadAll(const char* filename, ConfigInterface* config, CreateDeviceFn create_fn) {
  if (!config) {
    return false;
  }
  String content;
  if (!config->read_file(filename, &content)) {
    return false;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, content);
  if (error) {
    return false;
  }
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    create_fn(obj["id"], obj["name"], obj["mfg"], obj["type"], obj["timeout"]);
  }
  return true;
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
  m_last_packet_millis = millis();
  m_packet_count += 1;
}

void Device::addHAEntry(HADiscovery::Entry& entry, const char* sensor_name) {
  entry.device_name = cname();
  entry.device_id = cdevice_id();
  entry.manufacturer = manufacturer().c_str();
  if (!m_device_type.empty()) {
    entry.model = cdevice_type();
  }
  char entry_name[80];
  make_entry_name(entry_name, sizeof(entry_name), cname(), sensor_name);
  entry.entry_name = entry_name;
  entry.software = cname();
  entry.via_device = ha_discovery().deviceId();
  char availability[80];
  snprintf(availability, sizeof(availability), "~/%s_connection", cname());
  entry.availability = availability;
  // entry.icon
  JsonDocument json;
  // TODO(chrishl): should bookkeep and send again if this fails.
  ha_discovery().addEntry(&json, entry);
}
void Device::setAllSensorReadingsFailed() {
  for (auto& iter : m_id_to_float_sensor) {
    iter.second->set_failed();
  }
  for (auto& iter : m_id_to_int_sensor) {
    iter.second->set_failed();
  }
}

void Device::setIsOnline(bool is_online) {
  if (is_online == m_is_online) {
    return;
  }
  m_is_online = is_online;
  if (!is_online) {
    setAllSensorReadingsFailed();
  }
  auto mqtt = m_discovery->mqttManager();
  if (!mqtt) {
    return;
  }
  char availability[80];
  snprintf(availability, sizeof(availability), "%s_connection", cname());
  mqtt->mqttSend(mqtt->topic(availability).c_str(), is_online ? "online" : "offline");
}

bool Device::isTimedOut() const {
  if (!m_is_online) {
    return true;
  }
  const uint32_t elapsed = millis() - m_last_packet_millis;
  return elapsed > m_comms_timeout_millis;
}

}  // namespace og3::base_station
