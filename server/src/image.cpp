#include "image.h"

void
image::resize(std::size_t w, std::size_t h, std::size_t c)
{
  width = w;
  height = h;
  channels = c;
  data.resize(w * h * c);
}
