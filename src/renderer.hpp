#ifndef RENDERER_HPP
#define RENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <optional>

#ifndef NDEBUG
#define ENABLE_VALIDATION_LAYERS
#endif

struct Queue_family_indices
{
    std::uint32_t graphics;
    std::uint32_t present;
};

struct Swapchain
{
    vk::raii::SwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 tex_coord;
};

class Renderer
{
public:
    [[nodiscard]] Renderer(GLFWwindow *window,
                           std::uint32_t width,
                           std::uint32_t height);
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = default;
    Renderer &operator=(Renderer &&) = default;

private:
    [[nodiscard]] vk::raii::SurfaceKHR create_surface(GLFWwindow *window);

    [[nodiscard]] vk::raii::PhysicalDevice select_physical_device();

    [[nodiscard]] bool is_physical_device_suitable(
        const vk::raii::PhysicalDevice &physical_device);

    [[nodiscard]] std::optional<Queue_family_indices>
    get_queue_family_indices(const vk::raii::PhysicalDevice &physical_device);

    [[nodiscard]] vk::raii::Device create_device();

    [[nodiscard]] vk::raii::Queue get_graphics_queue();

    [[nodiscard]] vk::raii::Queue get_present_queue();

    [[nodiscard]] Swapchain create_swapchain(std::uint32_t width,
                                             std::uint32_t height);

    [[nodiscard]] std::vector<vk::Image> get_swapchain_images() const;

    [[nodiscard]] std::vector<vk::raii::ImageView>
    create_swapchain_image_views();

    [[nodiscard]] vk::raii::ImageView create_image_view(const vk::Image &image,
                                                        vk::Format format);

    [[nodiscard]] vk::raii::RenderPass create_render_pass();

    [[nodiscard]] vk::raii::DescriptorSetLayout create_descriptor_set_layout();

    [[nodiscard]] vk::raii::PipelineLayout create_pipeline_layout();

    [[nodiscard]] vk::raii::Pipeline create_pipeline();

    [[nodiscard]] vk::raii::ShaderModule
    create_shader_module(const std::vector<std::uint8_t> &shader_code);

    [[nodiscard]] std::vector<vk::raii::Framebuffer> create_framebuffers();

    vk::raii::Instance m_instance;
#ifdef ENABLE_VALIDATION_LAYERS
    vk::raii::DebugUtilsMessengerEXT m_debug_messenger;
#endif
    vk::raii::SurfaceKHR m_surface;
    vk::raii::PhysicalDevice m_physical_device;
    Queue_family_indices m_queue_family_indices;
    vk::raii::Device m_device;
    vk::raii::Queue m_graphics_queue;
    vk::raii::Queue m_present_queue;
    Swapchain m_swapchain;
    std::vector<vk::Image> m_swapchain_images;
    std::vector<vk::raii::ImageView> m_swapchain_image_views;
    vk::raii::RenderPass m_render_pass;
    vk::raii::DescriptorSetLayout m_descriptor_set_layout;
    vk::raii::PipelineLayout m_pipeline_layout;
    vk::raii::Pipeline m_pipeline;
    std::vector<vk::raii::Framebuffer> m_framebuffers;
};

#endif // RENDERER_HPP
