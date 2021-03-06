#ifndef RENDERER_HPP
#define RENDERER_HPP

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_SETTERS
#define VULKAN_HPP_NO_UNION_CONSTRUCTORS
#define VULKAN_HPP_NO_UNION_SETTERS
#include <vulkan/vulkan_raii.hpp>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <cstdint>
#include <optional>
#include <vector>

#ifndef NDEBUG
#define ENABLE_VALIDATION_LAYERS
#endif

#define ENABLE_DEBUG_UI

struct Queue_family_indices
{
    std::uint32_t graphics;
    std::uint32_t present;
};

struct Vulkan_swapchain
{
    vk::raii::SwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
    std::uint32_t min_image_count;
};

struct Vulkan_image
{
    vk::raii::Image image;
    vk::raii::ImageView view;
    vk::raii::DeviceMemory memory;
};

struct Vulkan_buffer
{
    vk::raii::Buffer buffer;
    vk::raii::DeviceMemory memory;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 tex_coord;
};

struct Vertex_array
{
    std::vector<Vertex> vertices;
    std::vector<std::uint16_t> indices;
};

struct Sync_objects
{
    std::vector<vk::raii::Semaphore> image_available_semaphores;
    std::vector<vk::raii::Semaphore> render_finished_semaphores;
    std::vector<vk::raii::Fence> in_flight_fences;
};

struct Push_constants
{
    glm::vec2 resolution;
    glm::vec2 mouse_position;
};

class Renderer
{
public:
    [[nodiscard]] Renderer(GLFWwindow *window,
                           std::uint32_t width,
                           std::uint32_t height);
    ~Renderer();

    void resize_framebuffer(std::uint32_t width, std::uint32_t height);

    void draw_frame(const glm::vec2 &mouse_position);

private:
    void record_command_buffer(std::uint32_t image_index,
                               const Push_constants &push_constants);

    [[nodiscard]] static Vertex_array create_vertex_array();

    [[nodiscard]] Sync_objects create_sync_objects();

    void recreate_swapchain();

    vk::raii::Context m_context;
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
    Vulkan_swapchain m_swapchain;
    std::vector<vk::Image> m_swapchain_images;
    std::vector<vk::raii::ImageView> m_swapchain_image_views;
    vk::raii::Sampler m_sampler;
    vk::raii::DescriptorPool m_descriptor_pool;
#ifdef ENABLE_DEBUG_UI
    vk::raii::DescriptorPool m_imgui_descriptor_pool;
#endif
    vk::raii::CommandPool m_command_pool;
    Vertex_array m_vertex_array;

    // Offscreen pass
    std::uint32_t m_offscreen_width;
    std::uint32_t m_offscreen_height;
    Vulkan_image m_offscreen_color_attachment;
    vk::raii::RenderPass m_offscreen_render_pass;
    vk::raii::DescriptorSetLayout m_offscreen_descriptor_set_layout;
    vk::raii::PipelineLayout m_offscreen_pipeline_layout;
    vk::raii::Pipeline m_offscreen_pipeline;
    vk::raii::Framebuffer m_offscreen_framebuffer;
    Vulkan_image m_offscreen_texture_image;
    Vulkan_buffer m_offscreen_vertex_buffer;
    Vulkan_buffer m_offscreen_index_buffer;
    vk::DescriptorSet m_offscreen_descriptor_set;

    // Final pass
    std::uint32_t m_framebuffer_width;
    std::uint32_t m_framebuffer_height;
    vk::raii::RenderPass m_render_pass;
    vk::raii::DescriptorSetLayout m_descriptor_set_layout;
    vk::raii::PipelineLayout m_pipeline_layout;
    vk::raii::Pipeline m_pipeline;
    std::vector<vk::raii::Framebuffer> m_framebuffers;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    vk::raii::CommandBuffers m_draw_command_buffers;
    Sync_objects m_sync_objects;
    std::uint32_t m_current_frame {};
    bool m_framebuffer_resized {};
};

#endif // RENDERER_HPP
