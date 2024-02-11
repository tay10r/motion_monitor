#pragma once

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <cstddef>
#include <cstdint>

namespace sentinel::proto {

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

class payload_visitor
{
public:
  virtual ~payload_visitor() = default;

  virtual void visit_rgb_camera_update(const std::uint8_t* data,
                                       std::uint16_t w,
                                       std::uint16_t h,
                                       std::uint64_t time,
                                       std::uint32_t sensor_id) = 0;

  virtual void visit_monochrome_camera_update(const std::uint8_t* data,
                                              std::uint16_t w,
                                              std::uint16_t h,
                                              std::uint64_t time,
                                              std::uint32_t sensor_id) = 0;

  virtual void visit_microphone_update(const std::int16_t* data,
                                       std::uint32_t size,
                                       std::uint32_t sample_rate,
                                       std::uint64_t time,
                                       std::uint32_t sensor_id) = 0;

  virtual void visit_temperature_update(const float temperature, std::uint64_t time, std::uint32_t sensor_id) = 0;

  /**
   * @brief Called when the payload is not able to be decoded.
   *
   * @return The derived class must return whether or not the payload was able to be decoded.
   *         This return value is propagated to the decoder function return value.
   * */
  virtual auto visit_unknown_payload(const std::string& type, const void* payload, std::size_t size) -> bool = 0;
};

class payload_visitor_base : public payload_visitor
{
public:
  ~payload_visitor_base() override = default;

  void visit_rgb_camera_update(const std::uint8_t* data,
                               std::uint16_t w,
                               std::uint16_t h,
                               std::uint64_t time,
                               std::uint32_t sensor_id) override
  {
  }

  void visit_monochrome_camera_update(const std::uint8_t* data,
                                      std::uint16_t w,
                                      std::uint16_t h,
                                      std::uint64_t time,
                                      std::uint32_t sensor_id) override
  {
  }

  void visit_microphone_update(const std::int16_t* data,
                               std::uint32_t size,
                               std::uint32_t sample_rate,
                               std::uint64_t time,
                               std::uint32_t sensor_id) override
  {
  }

  void visit_temperature_update(const float temperature, std::uint64_t time, std::uint32_t sensor_id) override {}

  auto visit_unknown_payload(const std::string& type, const void* payload, std::size_t size) -> bool override
  {
    return true;
  }
};

/**
 * @brief Attempts to decode the payload in a message.
 *
 * @param type The type indicates by the message header.
 *
 * @param payload The payload of the message.
 *
 * @param visitor The visitor to pass the decoded information to.
 *
 * @return True if the message was able to be decoded, false otherwise.
 * */
auto
decode_payload(const std::string& type, const void* payload, std::size_t payload_size, payload_visitor& visitor)
  -> bool;

struct outbound_message final
{
  /**
   * @brief The hash of the type string.
   * */
  std::size_t type_hash{};

  /**
   * @brief Whether or not outbound messages of a similar type can be conflated.
   * */
  bool conflate{ false };

  /**
   * @brief The data to send across the wire.
   * */
  std::shared_ptr<std::vector<std::uint8_t>> buffer;
};

/**
 * @brief Used for composing messages.
 * */
class writer final
{
public:
  static auto create_ready_update(std::uint64_t time) -> std::shared_ptr<outbound_message>;

  static auto create_rgb_camera_update(const std::uint8_t* data,
                                       std::uint16_t w,
                                       std::uint16_t h,
                                       std::uint64_t time,
                                       std::uint32_t sensor_id) -> std::shared_ptr<outbound_message>;

  static auto create_monochrome_camera_update(const std::uint8_t* data,
                                              std::uint16_t w,
                                              std::uint16_t h,
                                              std::uint64_t time,
                                              std::uint32_t sensor_id) -> std::shared_ptr<outbound_message>;

  static auto create_microphone_update(const std::int16_t* data,
                                       std::uint32_t size,
                                       std::uint32_t sample_rate,
                                       std::uint64_t time,
                                       std::uint32_t sensor_id) -> std::shared_ptr<outbound_message>;

  static auto create_temperature_update(float temperature, std::uint64_t time, std::uint32_t sensor_id)
    -> std::shared_ptr<outbound_message>;

  /**
   * @brief Constructs a new writer.
   *
   * @param type The type of the message being written.
   *
   * @param size The size of the payload.
   *
   * @param conflate Whether or not the type of message being constructed can be conflated.
   *
   * @note The finished message must be exactly the size of the payload.
   * */
  writer(const char* type, std::size_t size, bool conflate);

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
   * @return The buffer containing the message, which contains the message data as well as a few other fields for stream
   * control.
   * */
  auto complete() -> std::shared_ptr<outbound_message>;

private:
  std::vector<std::uint8_t> m_data;

  std::size_t m_offset{};

  std::size_t m_type_hash{};

  bool m_conflate{ false };
};

/**
 * @brief This class is used for controlling the amount of data that gets queued for sending out to clients.
 * */
class queue final
{
public:
  /**
   * @brief Constructs a new queue.
   *
   * @param max_messages_per_topic Up to this many messages are allowed to get queued for each topic.
   * */
  explicit queue(std::size_t max_messages_per_topic = 16);

  /**
   * @brief Removes all messages in the queue.
   * */
  void clear();

  /**
   * @brief Indicates whether or not there are any messages queued.
   * */
  auto empty() const -> bool { return m_queue.empty(); }

  /**
   * @brief Queues a message for sending out to a client.
   * */
  void add(std::shared_ptr<outbound_message> msg);

  /**
   * @brief Aggregates all messages into one.
   *
   * @note The caller should clear the queue after calling this function, in most circumstances.
   * */
  auto aggregate() const -> std::vector<std::uint8_t>;

protected:
  using map_type = std::map<std::size_t, std::vector<std::shared_ptr<outbound_message>>>;

private:
  map_type m_queue;

  std::size_t m_max_messages_per_topic{};
};

} // namespace sentinel::proto