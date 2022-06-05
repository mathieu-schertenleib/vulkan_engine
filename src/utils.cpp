#include "utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <fstream>
#include <iostream>

std::vector<std::uint8_t> load_binary_file(const std::filesystem::path &path)
{
    const auto size = std::filesystem::file_size(path);
    std::vector<std::uint8_t> data(size);

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to load binary file \"" << path << "\"\n";
        return {};
    }

    file.read(reinterpret_cast<char *>(data.data()),
              static_cast<std::streamsize>(size));

    return data;
}

Image load_image(const std::filesystem::path &path)
{
    int width {};
    int height {};
    int channels {};
    std::uint8_t *const pixels {stbi_load(
        path.generic_string().c_str(), &width, &height, &channels, 0)};

    if (!pixels)
    {
        std::cerr << "Failed to load image \"" << path << "\"\n";
        return {};
    }

    const auto size {static_cast<std::size_t>(width * height * channels)};
    std::vector<std::uint8_t> data(pixels, pixels + size);

    stbi_image_free(pixels);

    return {
        .width = width, .height = height, .channels = channels, .data = data};
}

void store_png(const std::filesystem::path &path, const Image &image)
{
    const auto stride = image.width * image.channels;
    const auto result = stbi_write_png(path.generic_string().c_str(),
                                       image.width,
                                       image.height,
                                       image.channels,
                                       image.data.data(),
                                       stride);
    if (result == 0)
    {
        std::cerr << "Failed to create PNG image \"" << path << "\"\n";
    }
}
