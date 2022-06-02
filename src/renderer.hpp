#ifndef RENDERER_HPP
#define RENDERER_HPP

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#ifndef NDEBUG
#define ENABLE_VALIDATION_LAYERS
#endif

class Renderer
{
public:
    [[nodiscard]] Renderer(
        const std::vector<const char *> &required_instance_extensions);

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = default;
    Renderer &operator=(Renderer &&) = default;

private:
    [[nodiscard]] static vk::raii::Instance create_instance(
        const std::vector<const char *> &required_instance_extensions);

#ifdef ENABLE_VALIDATION_LAYERS
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                   VkDebugUtilsMessageTypeFlagsEXT message_type,
                   const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
                   void *user_data);
    static constexpr vk::DebugUtilsMessengerCreateInfoEXT
        s_debug_utils_messenger_create_info {
            .messageSeverity =
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
            .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            .pfnUserCallback = debug_callback};
#endif

    [[nodiscard]] vk::raii::PhysicalDevice create_physical_device();
    [[nodiscard]] vk::raii::Device create_device();

    vk::raii::Instance m_instance;

#ifdef ENABLE_VALIDATION_LAYERS
    vk::raii::DebugUtilsMessengerEXT m_debug_messenger;
#endif

    vk::raii::PhysicalDevice m_physical_device;
    vk::raii::Device m_device;
};

#endif // RENDERER_HPP
