#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "vulkan_utils.hpp"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 tex_coord;
};

struct Sync_objects
{
    std::vector<vk::raii::Semaphore> image_available_semaphores;
    std::vector<vk::raii::Semaphore> render_finished_semaphores;
    std::vector<vk::raii::Fence> in_flight_fences;
};

class Renderer
{
public:
    [[nodiscard]] Renderer(GLFWwindow *window,
                           std::uint32_t width,
                           std::uint32_t height);
    ~Renderer();

    void resize_framebuffer(std::uint32_t width, std::uint32_t height);

    void draw_frame(float time, const glm::vec2 &mouse_position);

private:
    [[nodiscard]] Sync_objects create_sync_objects();

    void update_uniform_buffer(float time,
                               const glm::vec2 &mouse_position) const;

    void record_draw_command_buffer(std::uint32_t image_index);

    void recreate_swapchain();

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
    vk::raii::CommandPool m_command_pool;
    const std::vector<Vertex> m_vertices {{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
                                          {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                                          {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                                          {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}};
    const std::vector<std::uint16_t> m_indices {0, 1, 2, 2, 3, 0};

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
    Vulkan_buffer m_offscreen_uniform_buffer;
    vk::DescriptorSet m_offscreen_descriptor_set;

    // Final pass
    vk::raii::RenderPass m_render_pass;
    vk::raii::DescriptorSetLayout m_descriptor_set_layout;
    vk::raii::PipelineLayout m_pipeline_layout;
    vk::raii::Pipeline m_pipeline;
    std::vector<vk::raii::Framebuffer> m_framebuffers;
    Vulkan_buffer m_vertex_buffer;
    Vulkan_buffer m_index_buffer;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    vk::raii::CommandBuffers m_draw_command_buffers;
    Sync_objects m_sync_objects;
    std::uint32_t m_current_frame {};
    bool m_framebuffer_resized {};
    std::uint32_t m_framebuffer_width;
    std::uint32_t m_framebuffer_height;
};

#endif // RENDERER_HPP
