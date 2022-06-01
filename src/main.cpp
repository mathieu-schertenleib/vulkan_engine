#include "window.hpp"

#include <vulkan/vulkan.h>

#include <cstdlib>
#include <iostream>

int main()
{
    VkApplicationInfo application_info {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    VkInstanceCreateInfo instance_create_info {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    VkInstance instance {};
    const auto result =
        vkCreateInstance(&instance_create_info, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        std::cerr << "Failed to create Vulkan instance\n";
    }

    Window window;
    window.run();

    vkDestroyInstance(instance, nullptr);

    return EXIT_SUCCESS;
}
