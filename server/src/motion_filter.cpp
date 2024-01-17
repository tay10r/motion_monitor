#include "motion_filter.h"

namespace {

class motion_filter_impl final : public motion_filter
{
public:
  auto filter(const image& img) -> float override
  {
    //

    return 1.0f;
  }

private:
};

} // namespace

auto
motion_filter::create() -> std::unique_ptr<motion_filter>
{
  return std::make_unique<motion_filter_impl>();
}
