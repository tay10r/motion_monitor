#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

namespace motion_monitor {

class exception : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;

  ~exception() override = default;
};

class message_overflow_exception : public exception
{
public:
  using exception::exception;

  ~message_overflow_exception() override = default;
};

class message_incomplete_exception : public exception
{
public:
  using exception::exception;

  ~message_incomplete_exception() override = default;
};

/**
 * @brief Used to describe the results of a read operation.
 * */
struct read_result final
{
  /**
   * @brief How many bytes to remove from the beginning of the read buffer.
   * */
  std::size_t cull_size{};

  /**
   * @brief The type of the message that was read.
   * */
  std::string type_id;

  /**
   * @brief The byte offset to the beginning of the payload.
   * */
  std::size_t payload_offset{ 0 };

  /**
   * @brief The number of bytes in the payload.
   * */
  std::size_t payload_size{ 0 };

  /**
   * @brief Whether or not the payload in the message is complete.
   * */
  bool payload_ready{ false };
};

/**
 * @brief Attempts to extract a message from a read buffer.
 *
 * @return The result of the read operation.
 * */
auto
read(const std::uint8_t* data, std::size_t size) -> read_result;

/**
 * @brief Used for composing messages.
 * */
class writer final
{
public:
  /**
   * @brief Constructs a new writer.
   *
   * @param type The type of the message being written.
   *
   * @param size The size of the payload.
   *
   * @note The finished message must be exactly the size of the payload.
   * */
  writer(const char* type, std::size_t size);

  /**
   * @brief Writes content to the payload.
   *
   * @param data The data to write to the payload.
   *
   * @param size The number of bytes to write.
   * */
  void write(const void* data, std::size_t size);

  /**
   * @brief Completes the message and removes the buffer from the writer.
   *
   * @return The buffer containing the message, which can be sent as-is to the remote peer.
   * */
  auto complete() -> std::vector<std::uint8_t>;

private:
  std::vector<std::uint8_t> m_data;

  std::size_t m_offset{};
};

} // namespace motion_monitor
