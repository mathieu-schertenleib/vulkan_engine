#include "window.hpp"

#include <cstdlib>

int main()
{
    Window window;
    Renderer renderer(window.get_required_vulkan_instance_extensions());

    window.run();

    return EXIT_SUCCESS;
}
