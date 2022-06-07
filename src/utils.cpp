#include "utils.hpp"

#include <fstream>
#include <iostream>

std::vector<std::uint8_t> load_binary_file(const std::filesystem::path &path)
{
    if (!std::filesystem::exists(path))
    {
        std::cerr << "File " << path << " does not exist\n";
        return {};
    }

    const auto size = std::filesystem::file_size(path);
    std::vector<std::uint8_t> data(size);

    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open file " << path << '\n';
        return {};
    }

    file.read(reinterpret_cast<char *>(data.data()),
              static_cast<std::streamsize>(size));

    return data;
}
