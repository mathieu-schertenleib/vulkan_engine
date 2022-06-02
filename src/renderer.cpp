#include "renderer.hpp"

#include <iostream>
#include <vector>

Renderer::Renderer(
    const std::vector<const char *> &required_instance_extensions)
    : m_instance {create_instance(required_instance_extensions)}
#ifdef ENABLE_VALIDATION_LAYERS
    , m_debug_messenger {m_instance, s_debug_utils_messenger_create_info}
#endif
    , m_physical_device {create_physical_device()}
    , m_device {create_device()}
{
}

#ifdef ENABLE_VALIDATION_LAYERS
VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity [[maybe_unused]],
    VkDebugUtilsMessageTypeFlagsEXT message_type [[maybe_unused]],
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data [[maybe_unused]])
{
    std::cerr << callback_data->pMessage << std::endl;
    return VK_FALSE;
}
#endif

vk::raii::Instance Renderer::create_instance(
    const std::vector<const char *> &required_instance_extensions)
{
    constexpr vk::ApplicationInfo application_info {
        .pApplicationName = "Vulkan Experiment",
        .applicationVersion = {},
        .pEngineName = "Custom Engine",
        .engineVersion = {},
        .apiVersion = VK_API_VERSION_1_3};

#ifdef ENABLE_VALIDATION_LAYERS

    constexpr auto layer_name = "VK_LAYER_KHRONOS_validation";

    std::vector<const char *> enabled_extensions {required_instance_extensions};
    enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    const vk::InstanceCreateInfo instance_create_info {
        .pNext = &s_debug_utils_messenger_create_info,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = &layer_name,
        .enabledExtensionCount =
            static_cast<std::uint32_t>(enabled_extensions.size()),
        .ppEnabledExtensionNames = enabled_extensions.data()};

    return {{}, instance_create_info};

#else

    const vk::InstanceCreateInfo instance_create_info {
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount =
            static_cast<std::uint32_t>(required_instance_extensions.size()),
        .ppEnabledExtensionNames = required_instance_extensions.data()};
    return {{}, instance_create_info};

#endif
}

vk::raii::PhysicalDevice Renderer::create_physical_device()
{
    vk::raii::PhysicalDevices physical_devices(m_instance);
    return physical_devices.front();
}

vk::raii::Device Renderer::create_device()
{
    const vk::DeviceCreateInfo create_info {.queueCreateInfoCount = {},
                                            .pQueueCreateInfos = {},
                                            .enabledLayerCount = {},
                                            .ppEnabledLayerNames = {},
                                            .enabledExtensionCount = {},
                                            .ppEnabledExtensionNames = {},
                                            .pEnabledFeatures = {}};
    return {m_physical_device, create_info};
}
