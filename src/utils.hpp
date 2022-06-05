#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <filesystem>
#include <vector>

[[nodiscard]] std::vector<std::uint8_t>
load_binary_file(const std::filesystem::path &path);

struct Image
{
    int width;
    int height;
    int channels;
    std::vector<std::uint8_t> data;
};

[[nodiscard]] Image load_image(const std::filesystem::path &path);

void store_png(const std::filesystem::path &path, const Image &image);

#endif // UTILS_HPP
