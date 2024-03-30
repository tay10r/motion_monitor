#include <sentinel/proto.h>

namespace sentinel::proto {

queue::queue(std::size_t max_messages_per_topic)
  : m_max_messages_per_topic(max_messages_per_topic)
{
}

void
queue::clear()
{
  m_queue.clear();
}

void
queue::add(std::shared_ptr<outbound_message> msg)
{
  auto it = m_queue.find(msg->type_hash);

  if (it == m_queue.end()) {
    it = m_queue.emplace(msg->type_hash, std::vector<std::shared_ptr<outbound_message>>{}).first;
  }

  if (msg->conflate) {
    /* The message will be replaced. */
    it->second.clear();
  }

  auto& vec = it->second;

  if (vec.size() >= m_max_messages_per_topic) {
    vec.erase(vec.begin());
  }

  vec.emplace_back(msg);
}

auto
queue::aggregate() const -> std::shared_ptr<outbound_message>
{
  std::size_t size = 0;

  for (const auto& entry : m_queue) {

    for (const auto& msg : entry.second) {

      size += msg->buffer->size();
    }
  }

  writer wr("aggregate", size, false);

  for (const auto& entry : m_queue) {

    for (const auto& msg : entry.second) {

      wr.write(msg->buffer->data(), msg->buffer->size());
    }
  }

  return wr.complete();
}

} // namespace sentinel::proto
