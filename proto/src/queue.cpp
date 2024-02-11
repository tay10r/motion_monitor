#include <sentinel/proto.h>

namespace sentinel::proto {

queue::queue(std::size_t max_messages_per_topic)
  : m_max_messages_per_topic(max_messages_per_topic)
{
}

void
queue::add(std::shared_ptr<outbound_message> msg)
{
}

} // namespace sentinel::proto
