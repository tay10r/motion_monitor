#include <sentinel/proto.h>

#include <algorithm>
#include <functional>
#include <vector>

#include <cstring>

#include <stb_image_write.h>

namespace sentinel::proto {

namespace {

template<typename Scalar>
auto
clamp(Scalar x, Scalar min, Scalar max) -> Scalar
{
  return std::max(std::min(x, max), min);
}

} // namespace

writer::writer(const char* type, const std::size_t payload_size, const bool conflate)
  : m_type_hash(std::hash<std::string>{}(type /* note: string copy occurs here */))
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
writer::complete() -> std::shared_ptr<outbound_message>
{
  if (m_offset != m_data.size()) {
    throw message_incomplete_exception("Cannot finish a message with uninitialized data.");
  }

  auto msg = std::make_shared<outbound_message>();
  msg->type_hash = m_type_hash;
  msg->conflate = m_conflate;
  msg->buffer = std::make_shared<std::vector<std::uint8_t>>(std::move(m_data));
  return msg;
}

auto
writer::create_rgb_camera_update(const std::uint8_t* data,
                                 std::uint16_t w,
                                 std::uint16_t h,
                                 std::uint64_t time,
                                 std::uint32_t sensor_id,
                                 const std::vector<pixel_space_detection>& people,
                                 const float jpeg_quality) -> std::shared_ptr<outbound_message>
{
  const int quality = clamp(1 + static_cast<int>(jpeg_quality * 99), 1, 100);

  std::vector<std::uint8_t> buf;

  auto writer_func = [](void* buf_ptr, void* data, int size) {
    auto* buf = static_cast<std::vector<std::uint8_t>*>(buf_ptr);

    const auto prev_size = buf->size();

    buf->resize(prev_size + static_cast<std::size_t>(size));

    std::memcpy(&buf->at(prev_size), data, static_cast<std::size_t>(size));
  };

  stbi_write_jpg_to_func(writer_func, &buf, w, h, 3, data, quality);

  writer wr("rgb_camera::update",
            4 + 8 + 4 + buf.size() + pixel_space_detection::serialized_size() * people.size(),
            /* conflate */ true);

  const std::uint32_t buf_size = buf.size();

  wr.write(&buf_size, sizeof(buf_size));
  wr.write(&time, sizeof(time));
  wr.write(&sensor_id, sizeof(sensor_id));
  wr.write(buf.data(), buf.size());

  return wr.complete();
}

auto
writer::create_monochrome_camera_update(const std::uint8_t* data,
                                        std::uint16_t w,
                                        std::uint16_t h,
                                        std::uint64_t time,
                                        std::uint32_t sensor_id,
                                        const std::vector<pixel_space_detection>& people)
  -> std::shared_ptr<outbound_message>

{
  writer wr("monochrome_camera::update",
            2 + 2 + 8 + 4 + (w * h) + pixel_space_detection::serialized_size() * people.size(),
            /* conflate */ true);

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
                                 std::uint32_t sensor_id) -> std::shared_ptr<outbound_message>
{
  writer wr("microphone::update",
            size * 2 + sizeof(size) + sizeof(time) + sizeof(sample_rate) + sizeof(sensor_id),
            /* conflate */ false);

  wr.write(&sample_rate, sizeof(sample_rate));
  wr.write(&size, sizeof(size));
  wr.write(&time, sizeof(time));
  wr.write(&sensor_id, sizeof(sensor_id));
  wr.write(data, size * 2);

  return wr.complete();
}

auto
writer::create_temperature_update(float temperature, std::uint64_t time, std::uint32_t sensor_id)
  -> std::shared_ptr<outbound_message>
{
  writer wr("temperature::update", sizeof(float) + sizeof(time) + 4, /* conflate */ true);
  wr.write(&temperature, sizeof(temperature));
  wr.write(&time, sizeof(time));
  wr.write(&sensor_id, sizeof(sensor_id));
  return wr.complete();
}

auto
writer::create_ready_update(std::uint64_t time) -> std::shared_ptr<outbound_message>
{
  writer wr("ready", sizeof(time), /* conflate */ true);
  wr.write(&time, sizeof(time));
  return wr.complete();
}

} // namespace sentinel::proto
