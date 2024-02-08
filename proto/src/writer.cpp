#include <motion_monitor_proto.h>

#include <cstring>

namespace motion_monitor {

writer::writer(const char* type, const std::size_t payload_size)
{
  const std::size_t header_size = 8;

  const auto type_size = std::strlen(type);

  const auto total_size = header_size + type_size + payload_size;

  m_data.resize(total_size);

  const std::uint32_t header[2]{ static_cast<std::uint32_t>(type_size), static_cast<std::uint32_t>(payload_size) };

  static_assert(sizeof(header) == header_size, "Header size must be 8 bytes.");

  std::memcpy(&m_data[0], header, header_size);

  std::memcpy(&m_data[header_size], type, type_size);

  m_offset = header_size + type_size;
}

void
writer::write(const void* data, std::size_t size)
{
  if ((m_offset + size) > m_data.size()) {
    throw message_overflow_exception("Cannot write passed the end of the message.");
  }

  std::memcpy(&m_data[m_offset], data, size);

  m_offset += size;
}

auto
writer::complete() -> std::vector<std::uint8_t>
{
  if (m_offset != m_data.size()) {
    throw message_incomplete_exception("Cannot finish a message with uninitialized data.");
  }

  return std::move(m_data);
}

auto
writer::create_rgb_camera_update(const std::uint8_t* data,
                                 std::uint16_t w,
                                 std::uint16_t h,
                                 std::uint64_t time,
                                 std::uint32_t sensor_id) -> std::vector<std::uint8_t>
{
  writer wr("rgb_camera::update", 2 + 2 + 8 + 4 + (w * h * 3));

  wr.write(&w, sizeof(w));
  wr.write(&h, sizeof(h));
  wr.write(&time, sizeof(time));
  wr.write(&sensor_id, sizeof(sensor_id));
  wr.write(data, w * h * 3);

  return wr.complete();
}

auto
writer::create_monochrome_camera_update(const std::uint8_t* data,
                                        std::uint16_t w,
                                        std::uint16_t h,
                                        std::uint64_t time,
                                        std::uint32_t sensor_id) -> std::vector<std::uint8_t>

{
  writer wr("monochrome_camera::update", 2 + 2 + 8 + 4 + (w * h));

  wr.write(&w, sizeof(w));
  wr.write(&h, sizeof(h));
  wr.write(&time, sizeof(time));
  wr.write(&sensor_id, sizeof(sensor_id));
  wr.write(data, w * h);

  return wr.complete();
}

auto
writer::create_microphone_update(const std::int16_t* data,
                                 std::uint32_t size,
                                 std::uint32_t sample_rate,
                                 std::uint64_t time,
                                 std::uint32_t sensor_id) -> std::vector<std::uint8_t>
{
  writer wr("microphone::update", size * 2 + sizeof(size) + sizeof(time) + sizeof(sample_rate) + sizeof(sensor_id));

  wr.write(&sample_rate, sizeof(sample_rate));
  wr.write(&size, sizeof(size));
  wr.write(&time, sizeof(time));
  wr.write(&sensor_id, sizeof(sensor_id));
  wr.write(data, size * 2);

  return wr.complete();
}

auto
writer::create_temperature_update(float temperature, std::uint64_t time, std::uint32_t sensor_id)
  -> std::vector<std::uint8_t>
{
  writer wr("temperature::update", sizeof(float) + sizeof(time) + 4);
  wr.write(&temperature, sizeof(temperature));
  wr.write(&time, sizeof(time));
  wr.write(&sensor_id, sizeof(sensor_id));
  return wr.complete();
}

auto
writer::create_ready_update(std::uint64_t time) -> std::vector<std::uint8_t>
{
  writer wr("ready", sizeof(time));
  wr.write(&time, sizeof(time));
  return wr.complete();
}

} // namespace motion_monitor
