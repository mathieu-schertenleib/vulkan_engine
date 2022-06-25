#include "application.hpp"
#include "memory.hpp"

#include <cstdlib>
#include <iostream>

int main()
{
    try
    {
        Debug_memory_resource debug_memory_resource;
        // std::pmr::vector<int> vec {&debug_memory_resource};
        std::vector<int, std::pmr::polymorphic_allocator<int>> vec {
            &debug_memory_resource};
        vec.emplace_back();
        vec.emplace_back();
        vec.emplace_back();
        vec.emplace_back();
        vec.emplace_back();

        Application app;
        app.run();

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unknown exception thrown" << std::endl;
    }

    return EXIT_FAILURE;
}
