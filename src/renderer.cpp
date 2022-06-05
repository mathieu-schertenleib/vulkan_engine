#include "renderer.hpp"

#include <iostream>
#include <vector>

namespace
{

#ifdef ENABLE_VALIDATION_LAYERS

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity [[maybe_unused]],
    VkDebugUtilsMessageTypeFlagsEXT message_type [[maybe_unused]],
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data [[maybe_unused]])
{
    std::cerr << callback_data->pMessage << std::endl;
    return VK_FALSE;
}

inline constexpr vk::DebugUtilsMessengerCreateInfoEXT
    debug_utils_messenger_create_info {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = debug_callback};

inline constexpr auto validation_layer = "VK_LAYER_KHRONOS_validation";

#endif

[[nodiscard]] vk::raii::Instance
create_instance(const std::vector<const char *> &required_instance_extensions)
{
    constexpr vk::ApplicationInfo application_info {
        .pApplicationName = "Vulkan Experiment",
        .applicationVersion = {},
        .pEngineName = "Custom Engine",
        .engineVersion = {},
        .apiVersion = VK_API_VERSION_1_3};

#ifdef ENABLE_VALIDATION_LAYERS

    auto enabled_extensions = required_instance_extensions;
    enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    const vk::InstanceCreateInfo instance_create_info {
        .pNext = &debug_utils_messenger_create_info,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = &validation_layer,
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

[[nodiscard]] bool validation_layers_supported()
{
    const auto available_layers = vk::enumerateInstanceLayerProperties();

    return std::any_of(
        available_layers.begin(),
        available_layers.end(),
        [](const auto &layer)
        { return std::strcmp(layer.layerName, validation_layer) == 0; });
}

inline constexpr auto swapchain_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

[[nodiscard]] bool
swapchain_extension_supported(const vk::raii::PhysicalDevice &physical_device)
{
    const auto available_extensions =
        physical_device.enumerateDeviceExtensionProperties();

    return std::any_of(available_extensions.begin(),
                       available_extensions.end(),
                       [](const auto &extension) {
                           return std::strcmp(extension.extensionName,
                                              swapchain_extension) == 0;
                       });
}

} // namespace

Renderer::Renderer(
    const std::vector<const char *> &required_instance_extensions)
    : m_instance {create_instance(required_instance_extensions)}
#ifdef ENABLE_VALIDATION_LAYERS
    , m_debug_messenger {m_instance, debug_utils_messenger_create_info}
#endif
    , m_physical_device {select_physical_device()}
    , m_device {create_device()}
{
}

vk::raii::PhysicalDevice Renderer::select_physical_device()
{
    vk::raii::PhysicalDevices physical_devices(m_instance);
    const auto it =
        std::find_if(physical_devices.begin(),
                     physical_devices.end(),
                     [](const auto &physical_device)
                     {
                         const auto properties =
                             physical_device.getProperties();
                         return properties.deviceType ==
                                vk::PhysicalDeviceType::eDiscreteGpu;
                     });
    auto physical_device =
        (it != physical_devices.end()) ? *it : physical_devices.front();
    const auto properties = physical_device.getProperties();
    std::cout << "Device : " << properties.deviceName << '\n';

    return physical_device;
}

vk::raii::Device Renderer::create_device()
{
    constexpr float queue_priorities[] {1.0f};
    const vk::DeviceQueueCreateInfo queue_create_info {
        .queueFamilyIndex = compute_queue_family_index(),
        .queueCount = 1,
        .pQueuePriorities = queue_priorities};

    const vk::DeviceCreateInfo device_create_info {
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
#ifdef ENABLE_VALIDATION_LAYERS
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = &validation_layer,
#else
        .enabledLayerCount = {},
        .ppEnabledLayerNames = {},
#endif
        .enabledExtensionCount = {},
        .ppEnabledExtensionNames = {},
        .pEnabledFeatures = {}};

    return {m_physical_device, device_create_info};
}

std::uint32_t Renderer::compute_queue_family_index()
{
    const auto queue_family_properties =
        m_physical_device.getQueueFamilyProperties();
    auto properties_iterator =
        std::find_if(queue_family_properties.begin(),
                     queue_family_properties.end(),
                     [](const vk::QueueFamilyProperties &prop)
                     { return prop.queueFlags & vk::QueueFlagBits::eCompute; });
    return static_cast<std::uint32_t>(
        std::distance(queue_family_properties.begin(), properties_iterator));
}