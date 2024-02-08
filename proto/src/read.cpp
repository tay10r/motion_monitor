#include <motion_monitor_proto.h>

namespace motion_monitor {

namespace {

auto
unpack_u32(const std::uint8_t* data) -> std::uint32_t
{
  return *reinterpret_cast<const std::uint32_t*>(data);
}

} // namespace

auto
read(const std::uint8_t* data, const std::size_t size) -> read_result
{
  const std::size_t header_size{ 8 };

  if (size < header_size) {
    return read_result{};
  }

  const auto type_size = unpack_u32(data);

  const auto payload_size = unpack_u32(data + 4);

  if ((type_size + payload_size + header_size) > size) {
    return read_result{};
  }

  read_result result{};
  result.cull_size = header_size + type_size + payload_size;
  result.payload_ready = true;
  result.payload_offset = type_size + header_size;
  result.payload_size = payload_size;
  result.type_id = std::string(reinterpret_cast<const char*>(data + header_size), type_size);

  return result;
}

namespace {

auto
u16(const std::uint8_t* ptr) -> std::uint16_t
{
  return *reinterpret_cast<const std::uint16_t*>(ptr);
}

auto
u32(const std::uint8_t* ptr) -> std::uint32_t
{
  return *reinterpret_cast<const std::uint32_t*>(ptr);
}

auto
u64(const std::uint8_t* ptr) -> std::uint64_t
{
  return *reinterpret_cast<const std::uint64_t*>(ptr);
}

auto
f32(const std::uint8_t* ptr) -> float
{
  return *reinterpret_cast<const float*>(ptr);
}

} // namespace

auto
decode_payload(const std::string& type, const void* payload, std::size_t payload_size, payload_visitor& visitor) -> bool
{
  const std::uint8_t* ptr = static_cast<const std::uint8_t*>(payload);

  if (type == "rgb_camera::update") {
    const auto w = u16(ptr);
    const auto h = u16(ptr + 2);
    const auto t = u64(ptr + 4);
    const auto id = u32(ptr + 12);
    visitor.visit_rgb_camera_update(ptr + 16, w, h, t, id);
  } else if (type == "monochrome_camera::update") {
    const auto w = u16(ptr);
    const auto h = u16(ptr + 2);
    const auto t = u64(ptr + 4);
    const auto id = u32(ptr + 12);
    visitor.visit_rgb_camera_update(ptr + 16, w, h, t, id);
  } else if (type == "microphone::update") {
    const auto r = u32(ptr);
    const auto s = u32(ptr + 4);
    const auto t = u64(ptr + 8);
    const auto id = u32(ptr + 16);
    visitor.visit_microphone_update(reinterpret_cast<const std::int16_t*>(ptr + 20), s, r, t, id);
  } else if (type == "temperature::update") {
    const auto v = f32(ptr);
    const auto t = u64(ptr + 4);
    const auto id = u32(ptr + 12);
    visitor.visit_temperature_update(v, t, id);
  } else {
    return visitor.visit_unknown_payload(type, payload, payload_size);
  }

  return true;
}

} // namespace motion_monitor
