#include "renderer.hpp"

#include "utils.hpp"

namespace
{

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 tex_coord;
};

struct Uniform_buffer_object
{
    glm::vec2 resolution;
    glm::vec2 mouse_position;
    float time;
    float delta_time;
};

inline constexpr vk::VertexInputBindingDescription vertex_binding_description {
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = vk::VertexInputRate::eVertex};

inline constexpr std::array vertex_attribute_descriptions {
    vk::VertexInputAttributeDescription {.location = 0,
                                         .binding = 0,
                                         .format = vk::Format::eR32G32B32Sfloat,
                                         .offset = offsetof(Vertex, pos)},
    vk::VertexInputAttributeDescription {.location = 1,
                                         .binding = 0,
                                         .format = vk::Format::eR32G32Sfloat,
                                         .offset =
                                             offsetof(Vertex, tex_coord)}};

/*
    0---3
    | \ |
    1---2
 */
const std::vector<Vertex> vertices {{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
                                    {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                                    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
                                    {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}};

const std::vector<std::uint16_t> indices {0, 1, 2, 2, 3, 0};

} // namespace

Renderer::Renderer(GLFWwindow *window,
                   std::uint32_t width,
                   std::uint32_t height)
    : m_instance {create_instance()}
#ifdef ENABLE_VALIDATION_LAYERS
    , m_debug_messenger {create_debug_utils_messenger(m_instance)}
#endif
    , m_surface {create_surface(m_instance, window)}
    , m_physical_device {select_physical_device(m_instance, *m_surface)}
    , m_queue_family_indices {get_queue_family_indices(m_physical_device,
                                                       *m_surface)
                                  .value()}
    , m_device {create_device(m_physical_device, m_queue_family_indices)}
    , m_graphics_queue {m_device.getQueue(m_queue_family_indices.graphics, 0)}
    , m_present_queue {m_device.getQueue(m_queue_family_indices.present, 0)}
    , m_swapchain {create_swapchain(m_device,
                                    m_physical_device,
                                    *m_surface,
                                    m_queue_family_indices,
                                    width,
                                    height,
                                    true)}
    , m_swapchain_images {get_swapchain_images(m_swapchain.swapchain)}
    , m_swapchain_image_views {create_swapchain_image_views(
          m_device, m_swapchain_images, m_swapchain.format)}
    , m_render_pass {create_render_pass(m_device, m_swapchain.format)}
    , m_descriptor_set_layout {create_descriptor_set_layout(m_device)}
    , m_pipeline_layout {create_pipeline_layout(m_device,
                                                m_descriptor_set_layout)}
    , m_pipeline {create_pipeline(m_device,
                                  "shaders/spv/shader.vert.spv",
                                  "shaders/spv/shader.frag.spv",
                                  m_swapchain.extent,
                                  *m_pipeline_layout,
                                  *m_render_pass,
                                  vertex_binding_description,
                                  vertex_attribute_descriptions.data(),
                                  vertex_attribute_descriptions.size())}
    , m_framebuffers {create_framebuffers(m_device,
                                          m_swapchain_image_views,
                                          *m_render_pass,
                                          m_swapchain.extent)}
    , m_command_pool {create_command_pool(m_device,
                                          m_queue_family_indices.graphics)}
    , m_sampler {create_sampler(m_device)}
    , m_texture_image {create_texture_image(m_device,
                                            m_physical_device,
                                            m_command_pool,
                                            m_graphics_queue,
                                            "assets/ray_traced_scene.png")}
    , m_vertex_buffer {create_vertex_buffer(m_device,
                                            m_physical_device,
                                            m_command_pool,
                                            m_graphics_queue,
                                            vertices.data(),
                                            vertices.size() * sizeof(Vertex))}
    , m_index_buffer {create_index_buffer(m_device,
                                          m_physical_device,
                                          m_command_pool,
                                          m_graphics_queue,
                                          indices)}
    , m_uniform_buffers {create_uniform_buffers(
          m_device, m_physical_device, sizeof(Uniform_buffer_object))}
    , m_descriptor_pool {create_descriptor_pool(m_device)}
    , m_descriptor_sets {create_descriptor_sets(m_device,
                                                *m_descriptor_set_layout,
                                                *m_descriptor_pool,
                                                *m_sampler,
                                                *m_texture_image.view,
                                                m_uniform_buffers,
                                                sizeof(Uniform_buffer_object))}
    , m_command_buffers {create_frame_command_buffers(m_device, m_command_pool)}
    , m_sync_objects {create_sync_objects()}
    , m_framebuffer_width {width}
    , m_framebuffer_height {height}
{
}

Renderer::~Renderer()
{
    m_device.waitIdle();
}

Sync_objects Renderer::create_sync_objects()
{
    Sync_objects result;

    for (std::size_t i {}; i < max_frames_in_flight; ++i)
    {
        constexpr vk::SemaphoreCreateInfo semaphore_create_info {};

        constexpr vk::FenceCreateInfo fence_create_info {
            .flags = vk::FenceCreateFlagBits::eSignaled};

        result.image_available_semaphores.emplace_back(m_device,
                                                       semaphore_create_info);
        result.render_finished_semaphores.emplace_back(m_device,
                                                       semaphore_create_info);
        result.in_flight_fences.emplace_back(m_device, fence_create_info);
    }

    return result;
}

void Renderer::update_uniform_buffer(std::uint32_t current_image,
                                     float time,
                                     float delta_time,
                                     const glm::vec2 &mouse_position)
{
    const Uniform_buffer_object ubo {
        .resolution = {static_cast<float>(m_framebuffer_width),
                       static_cast<float>(m_framebuffer_height)},
        .mouse_position = mouse_position,
        .time = time,
        .delta_time = delta_time};

    auto *const data =
        m_uniform_buffers[current_image].memory.mapMemory(0, sizeof(ubo));
    std::memcpy(data, &ubo, sizeof(ubo));
    m_uniform_buffers[current_image].memory.unmapMemory();
}

void Renderer::recreate_swapchain()
{
    m_device.waitIdle();

    m_swapchain = create_swapchain(m_device,
                                   m_physical_device,
                                   *m_surface,
                                   m_queue_family_indices,
                                   m_framebuffer_width,
                                   m_framebuffer_height,
                                   true);
    m_swapchain_images = get_swapchain_images(m_swapchain.swapchain);
    m_swapchain_image_views = create_swapchain_image_views(
        m_device, m_swapchain_images, m_swapchain.format);
    m_render_pass = create_render_pass(m_device, m_swapchain.format);
    m_pipeline_layout =
        create_pipeline_layout(m_device, m_descriptor_set_layout);
    m_pipeline = create_pipeline(m_device,
                                 "shaders/spv/shader.vert.spv",
                                 "shaders/spv/shader.frag.spv",
                                 m_swapchain.extent,
                                 *m_pipeline_layout,
                                 *m_render_pass,
                                 vertex_binding_description,
                                 vertex_attribute_descriptions.data(),
                                 vertex_attribute_descriptions.size());
    m_framebuffers = create_framebuffers(
        m_device, m_swapchain_image_views, *m_render_pass, m_swapchain.extent);
}

void Renderer::resize_framebuffer(std::uint32_t width, std::uint32_t height)
{
    m_framebuffer_resized = true;
    m_framebuffer_width = width;
    m_framebuffer_height = height;
}

void Renderer::draw_frame(float time,
                          float delta_time,
                          const glm::vec2 &mouse_position)
{
    const auto wait_result = m_device.waitForFences(
        *m_sync_objects.in_flight_fences[m_current_frame],
        VK_TRUE,
        std::numeric_limits<std::uint64_t>::max());
    if (wait_result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Error while waiting for fences");
    }

    const auto &[result, image_index] = m_swapchain.swapchain.acquireNextImage(
        std::numeric_limits<std::uint64_t>::max(),
        *m_sync_objects.image_available_semaphores[m_current_frame]);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        recreate_swapchain();
        return;
    }
    else if (result != vk::Result::eSuccess &&
             result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    update_uniform_buffer(m_current_frame, time, delta_time, mouse_position);

    m_device.resetFences(*m_sync_objects.in_flight_fences[m_current_frame]);

    m_command_buffers[m_current_frame].reset();

    record_frame_command_buffer(m_command_buffers[m_current_frame],
                                *m_render_pass,
                                *m_framebuffers[image_index],
                                m_swapchain.extent,
                                *m_pipeline,
                                *m_pipeline_layout,
                                m_descriptor_sets[m_current_frame],
                                *m_vertex_buffer.buffer,
                                *m_index_buffer.buffer,
                                static_cast<std::uint32_t>(indices.size()));

    const vk::PipelineStageFlags wait_stages[] {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};

    const vk::SubmitInfo submit_info {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &*m_sync_objects.image_available_semaphores[m_current_frame],
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &*m_command_buffers[m_current_frame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores =
            &*m_sync_objects.render_finished_semaphores[m_current_frame]};

    m_graphics_queue.submit(submit_info,
                            *m_sync_objects.in_flight_fences[m_current_frame]);

    const vk::PresentInfoKHR present_info {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &*m_sync_objects.render_finished_semaphores[m_current_frame],
        .swapchainCount = 1,
        .pSwapchains = &*m_swapchain.swapchain,
        .pImageIndices = &image_index};

    const auto present_result = m_present_queue.presentKHR(present_info);

    if (present_result == vk::Result::eErrorOutOfDateKHR ||
        present_result == vk::Result::eSuboptimalKHR || m_framebuffer_resized)
    {
        m_framebuffer_resized = false;
        recreate_swapchain();
    }
    else if (present_result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swapchain image");
    }

    m_current_frame = (m_current_frame + 1) % max_frames_in_flight;
}