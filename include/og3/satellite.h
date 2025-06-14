// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/adc_voltage.h>
#include <og3/units.h>
#include <og3/variable.h>

#include <memory>
#include <vector>

#include "og3/satellite.pb.h"

namespace og3::satellite {

class PacketReading {
 public:
  PacketReading(unsigned sensor_id, og3_Sensor_Type sensor_type, og3_Sensor_StateClass state_class)
      : m_sensor_id(sensor_id), m_sensor_type(sensor_type), m_state_class(state_class) {}

  virtual bool read() = 0;
  virtual bool write(og3_Packet& packet) = 0;
  virtual bool write_desc(og3_Packet& packet) = 0;

 protected:
  const unsigned m_sensor_id;
  const og3_Sensor_Type m_sensor_type;
  const og3_Sensor_StateClass m_state_class;
};

class PacketFloatReading : public PacketReading {
 public:
  PacketFloatReading(
      unsigned sensor_id, og3_Sensor_Type sensor_type, const FloatVariable& var,
      og3_Sensor_StateClass state_class = og3_Sensor_StateClass_STATE_CLASS_UNSPECIFIED)
      : PacketReading(sensor_id, sensor_type, state_class), m_var(var) {}

  bool write(og3_Packet& packet) override;
  bool write_desc(og3_Packet& packet) override;

 private:
  const FloatVariable& m_var;
};

class PacketVoltageReading : public PacketFloatReading {
 public:
  PacketVoltageReading(
      unsigned sensor_id, AdcVoltage& adc,
      og3_Sensor_StateClass state_class = og3_Sensor_StateClass_STATE_CLASS_UNSPECIFIED)
      : PacketFloatReading(sensor_id, og3_Sensor_Type_TYPE_VOLTAGE, adc.valueVariable(),
                           state_class),
        m_adc(adc) {}

  bool read() final {
    m_adc.read();
    return true;
  }

 private:
  AdcVoltage& m_adc;
};

class PacketIntReading : public PacketReading {
 public:
  PacketIntReading(
      unsigned sensor_id, const char* desc, Variable<unsigned>& ivar,
      og3_Sensor_StateClass state_class = og3_Sensor_StateClass_STATE_CLASS_UNSPECIFIED)
      : PacketReading(sensor_id, og3_Sensor_Type_TYPE_UNSPECIFIED, state_class),
        m_desc(desc),
        m_ivar(ivar) {}

  bool read() override { return true; }
  bool write(og3_Packet& packet) override;
  bool write_desc(og3_Packet& packet) override;

 private:
  const char* m_desc;
  Variable<unsigned>& m_ivar;
};

class PacketSender {
 public:
  // Data to be kept in RTC memory
  struct Rtc {
    uint16_t seq_id;
    unsigned secs_device_sent;
    unsigned sensor_descriptions_sent;
  };

  void send_desc(size_t max_size);
  void send_all_readings();
  bool is_sending() const { return m_is_sending; }
  void set_is_sending(bool is_sending) { m_is_sending = is_sending; }
  void set_board_id(uint32_t board_id) { m_board_id = board_id; }

 protected:
  PacketSender(const og3_Device* device, App* app, Rtc* rtc)
      : m_device(device), m_app(app), m_rtc(rtc) {}
  void start_packet(og3_Packet& packet, bool update_device);
  virtual void send_packet(og3_Packet& packet) = 0;

  const og3_Device* m_device;
  App* m_app;
  Rtc* m_rtc;
  std::vector<std::unique_ptr<PacketReading>> m_readings;
  bool m_is_sending = false;
  uint32_t m_board_id = 0xFFFF;
};

}  // namespace og3::satellite
