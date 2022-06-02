#include "utils.hpp"

#include <fstream>

std::vector<std::uint8_t> load_binary_file(const std::filesystem::path &path)
{
    const auto file_size = std::filesystem::file_size(path);
    std::vector<std::uint8_t> data(file_size);

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        return {};
    }

    file.read(reinterpret_cast<char *>(data.data()),
              static_cast<std::streamsize>(file_size));

    return data;
}
