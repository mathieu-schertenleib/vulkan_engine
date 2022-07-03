#include "memory.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
    std::cout << "sizeof(vk::raii::Device) = " << sizeof(vk::raii::Device)
              << "\nsizeof(vk::UniqueDevice) = " << sizeof(vk::UniqueDevice)
              << '\n';

    return EXIT_SUCCESS;
}