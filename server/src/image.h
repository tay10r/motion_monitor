#pragma once

#include <vector>

#include <cstdint>

struct image final
{
  std::size_t width{};

  std::size_t height{};

  std::size_t channels{};

  std::vector<std::uint8_t> data;

  void resize(std::size_t w, std::size_t h, std::size_t c);
};
