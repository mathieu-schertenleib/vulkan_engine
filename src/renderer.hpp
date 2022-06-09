#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "vulkan_utils.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/*
 * TODO:
 *  - Render to an offscreen (lower resolution) image, then blit it to the
 *    swapchain
 *  - Integrate ImGui
 */

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

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Renderer(Renderer &&) = default;
    Renderer &operator=(Renderer &&) = default;

    void resize_framebuffer(std::uint32_t width, std::uint32_t height);

    void
    draw_frame(float time, float delta_time, const glm::vec2 &mouse_position);

private:
    [[nodiscard]] Sync_objects create_sync_objects();

    void update_uniform_buffer(std::uint32_t current_image,
                               float time,
                               float delta_time,
                               const glm::vec2 &mouse_position);

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
    Swapchain m_swapchain;
    std::vector<vk::Image> m_swapchain_images;
    std::vector<vk::raii::ImageView> m_swapchain_image_views;
    vk::raii::RenderPass m_render_pass;
    vk::raii::DescriptorSetLayout m_descriptor_set_layout;
    vk::raii::PipelineLayout m_pipeline_layout;
    vk::raii::Pipeline m_pipeline;
    std::vector<vk::raii::Framebuffer> m_framebuffers;
    vk::raii::CommandPool m_command_pool;
    vk::raii::Sampler m_sampler;
    Image m_texture_image;
    Buffer m_vertex_buffer;
    Buffer m_index_buffer;
    std::vector<Buffer> m_uniform_buffers;
    vk::raii::DescriptorPool m_descriptor_pool;
    std::vector<vk::DescriptorSet> m_descriptor_sets;
    vk::raii::CommandBuffers m_command_buffers;
    Sync_objects m_sync_objects;
    std::uint32_t m_current_frame {};
    bool m_framebuffer_resized {};
    std::uint32_t m_framebuffer_width;
    std::uint32_t m_framebuffer_height;
};

#endif // RENDERER_HPP
