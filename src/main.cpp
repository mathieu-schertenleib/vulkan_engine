#include "application.hpp"
#include "memory.hpp"

#include <cstdlib>
#include <iostream>

/*
 * Notes:
 *
 * - vk::raii types like vk::raii::Device use new for allocating a dispatcher
 * storing all function pointers
 *
 * - std::vectors used in vk::raii functions can not use custom allocators, as
 * opposed to vk::UniqueHandle
 *
 * - for the Vulkan memory management, we really should use
 * VulkanMemoryAllocator
 *
 */

int main()
{
    try
    {
        Application app;
        app.run();

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unknown exception thrown\n";
    }

    return EXIT_FAILURE;
}
