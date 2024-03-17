#include <sentinel/proto.h>

#include <optional>

#include <stb_image.h>

namespace sentinel::proto {

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

auto
decode_camera_frame_event(const uint8_t* payload, const std::size_t size, const std::size_t channels)
  -> std::optional<camera_frame_event>
{
  const auto buf_size = u32(payload);

  if ((buf_size + 16) != size) {
    return {};
  }

  int w = 0;
  int h = 0;
  auto* ptr = stbi_load_from_memory(payload + 16, buf_size, &w, &h, nullptr, 3);
  if (ptr == nullptr) {
    return {};
  }

  camera_frame_event ev{};
  ev.allocated_pixel_data.reset(ptr);
  ev.w = w;
  ev.h = h;
  ev.time = u64(payload + 4);
  ev.sensor_id = u32(payload + 12);
  ev.data = ptr;
  return ev;
}

} // namespace

auto
decode_payload(const std::string& type, const void* payload, const std::size_t payload_size, payload_visitor& visitor)
  -> bool
{
  const std::uint8_t* ptr = static_cast<const std::uint8_t*>(payload);

  if (type == "rgb_camera::update") {
    const auto ev = decode_camera_frame_event(ptr, payload_size, 3);
    if (!ev) {
      return false;
    }
    visitor.visit_rgb_camera_frame_event(ev.value());
  } else if (type == "monochrome_camera::update") {
    const auto ev = decode_camera_frame_event(ptr, payload_size, 1);
    if (!ev) {
      return false;
    }
    visitor.visit_monochrome_camera_frame_event(ev.value());
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
  } else if (type == "aggregate") {
    for (std::size_t i = 0; i < payload_size;) {
      const auto res = read(ptr + i, payload_size - i);
      if (!res.payload_ready) {
        break;
      }
      const auto sub_ptr = static_cast<const std::uint8_t*>(payload) + i + res.payload_offset;
      if (!decode_payload(res.type_id, sub_ptr, res.payload_size, visitor)) {
        break;
      }
      i += res.payload_offset + res.payload_size;
    }
  } else {
    return visitor.visit_unknown_payload(type, payload, payload_size);
  }

  return true;
}

} // namespace sentinel::proto
