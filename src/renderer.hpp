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
    [[nodiscard]] explicit Renderer(
        const std::vector<const char *> &required_instance_extensions);

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = default;
    Renderer &operator=(Renderer &&) = default;

private:
    [[nodiscard]] vk::raii::PhysicalDevice select_physical_device();
    [[nodiscard]] vk::raii::Device create_device();

    [[nodiscard]] std::uint32_t compute_queue_family_index();

    vk::raii::Instance m_instance;
#ifdef ENABLE_VALIDATION_LAYERS
    vk::raii::DebugUtilsMessengerEXT m_debug_messenger;
#endif
    vk::raii::PhysicalDevice m_physical_device;
    vk::raii::Device m_device;
};

#endif // RENDERER_HPP
