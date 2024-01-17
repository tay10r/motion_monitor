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

} // namespace motion_monitor
