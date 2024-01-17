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

} // namespace motion_monitor
