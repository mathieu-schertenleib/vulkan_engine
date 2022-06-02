#ifndef UTILS_HPP
#define UTILS_HPP

#include <filesystem>
#include <vector>

[[nodiscard]] std::vector<std::uint8_t>
load_binary_file(const std::filesystem::path &path);

#endif // UTILS_HPP
