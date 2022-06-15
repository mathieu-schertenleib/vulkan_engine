#ifndef VULKAN_UTILS_HPP
#define VULKAN_UTILS_HPP

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

#include <cstdint>
#include <optional>
#include <vector>

#ifndef NDEBUG
#define ENABLE_VALIDATION_LAYERS
#endif

inline constexpr std::uint32_t max_frames_in_flight {2};

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

[[nodiscard]] vk::raii::Instance create_instance();

#ifdef ENABLE_VALIDATION_LAYERS
[[nodiscard]] vk::raii::DebugUtilsMessengerEXT
create_debug_utils_messenger(const vk::raii::Instance &instance);
#endif

[[nodiscard]] vk::raii::SurfaceKHR
create_surface(const vk::raii::Instance &instance, GLFWwindow *window);

[[nodiscard]] vk::raii::PhysicalDevice
select_physical_device(const vk::raii::Instance &instance,
                       vk::SurfaceKHR surface);

[[nodiscard]] std::optional<Queue_family_indices>
get_queue_family_indices(const vk::raii::PhysicalDevice &physical_device,
                         vk::SurfaceKHR surface);

[[nodiscard]] vk::raii::Device
create_device(const vk::raii::PhysicalDevice &physical_device,
              const Queue_family_indices &queue_family_indices);

[[nodiscard]] Vulkan_swapchain
create_swapchain(const vk::raii::Device &device,
                 const vk::raii::PhysicalDevice &physical_device,
                 vk::SurfaceKHR surface,
                 const Queue_family_indices &queue_family_indices,
                 std::uint32_t width,
                 std::uint32_t height);

[[nodiscard]] std::vector<vk::Image>
get_swapchain_images(const vk::raii::SwapchainKHR &swapchain);

[[nodiscard]] std::vector<vk::raii::ImageView>
create_swapchain_image_views(const vk::raii::Device &device,
                             const std::vector<vk::Image> &swapchain_images,
                             vk::Format swapchain_format);

[[nodiscard]] vk::raii::ImageView create_image_view(
    const vk::raii::Device &device, vk::Image image, vk::Format format);

[[nodiscard]] vk::raii::CommandPool
create_command_pool(const vk::raii::Device &device,
                    std::uint32_t graphics_family_index);

[[nodiscard]] vk::raii::CommandBuffer
begin_one_time_submit_command_buffer(const vk::raii::Device &device,
                                     const vk::raii::CommandPool &command_pool);

void end_one_time_submit_command_buffer(
    const vk::raii::CommandBuffer &command_buffer,
    const vk::raii::Queue &graphics_queue);

[[nodiscard]] std::uint32_t
find_memory_type(const vk::raii::PhysicalDevice &physical_device,
                 std::uint32_t type_filter,
                 vk::MemoryPropertyFlags requested_properties);

[[nodiscard]] Vulkan_buffer
create_buffer(const vk::raii::Device &device,
              const vk::raii::PhysicalDevice &physical_device,
              vk::DeviceSize size,
              vk::BufferUsageFlags usage,
              vk::MemoryPropertyFlags properties);

[[nodiscard]] Vulkan_image
create_image(const vk::raii::Device &device,
             const vk::raii::PhysicalDevice &physical_device,
             std::uint32_t width,
             std::uint32_t height,
             vk::Format format,
             vk::ImageUsageFlags usage,
             vk::MemoryPropertyFlags properties);

void command_copy_buffer(const vk::raii::CommandBuffer &command_buffer,
                         vk::Buffer src,
                         vk::Buffer dst,
                         vk::DeviceSize size);

void command_copy_buffer_to_image(const vk::raii::CommandBuffer &command_buffer,
                                  vk::Buffer buffer,
                                  vk::Image image,
                                  std::uint32_t width,
                                  std::uint32_t height);

void command_copy_image_to_buffer(const vk::raii::CommandBuffer &command_buffer,
                                  vk::Image image,
                                  vk::Buffer buffer,
                                  std::uint32_t width,
                                  std::uint32_t height);

void command_blit_image(const vk::raii::CommandBuffer &command_buffer,
                        vk::Image src_image,
                        std::int32_t src_width,
                        std::int32_t src_height,
                        vk::Image dst_image,
                        std::int32_t dst_width,
                        std::int32_t dst_height);

void command_transition_image_layout(
    const vk::raii::CommandBuffer &command_buffer,
    vk::Image image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::PipelineStageFlags src_stage_mask,
    vk::PipelineStageFlags dst_stage_mask,
    vk::AccessFlags src_access_mask,
    vk::AccessFlags dst_access_mask);

[[nodiscard]] vk::raii::RenderPass
create_render_pass(const vk::raii::Device &device,
                   vk::Format color_attachment_format,
                   vk::ImageLayout final_color_attachment_layout);

[[nodiscard]] vk::raii::DescriptorSetLayout
create_descriptor_set_layout(const vk::raii::Device &device);

[[nodiscard]] vk::raii::PipelineLayout create_pipeline_layout(
    const vk::raii::Device &device,
    const vk::raii::DescriptorSetLayout &descriptor_set_layout);

[[nodiscard]] vk::raii::Pipeline create_pipeline(
    const vk::raii::Device &device,
    const char *vertex_shader_path,
    const char *fragment_shader_path,
    const vk::Extent2D &swapchain_extent,
    vk::PipelineLayout pipeline_layout,
    vk::RenderPass render_pass,
    const vk::VertexInputBindingDescription &vertex_binding_description,
    const vk::VertexInputAttributeDescription *vertex_attribute_descriptions,
    std::uint32_t num_vertex_attribute_descriptions);

[[nodiscard]] vk::raii::ShaderModule
create_shader_module(const vk::raii::Device &device,
                     const std::vector<std::uint8_t> &shader_code);

[[nodiscard]] vk::raii::Framebuffer
create_framebuffer(const vk::raii::Device &device,
                   const vk::raii::ImageView &image_view,
                   vk::RenderPass render_pass,
                   std::uint32_t width,
                   std::uint32_t height);

[[nodiscard]] std::vector<vk::raii::Framebuffer>
create_framebuffers(const vk::raii::Device &device,
                    const std::vector<vk::raii::ImageView> &image_views,
                    vk::RenderPass render_pass,
                    std::uint32_t width,
                    std::uint32_t height);

[[nodiscard]] vk::raii::Sampler create_sampler(const vk::raii::Device &device);

[[nodiscard]] Vulkan_image
create_texture_image(const vk::raii::Device &device,
                     const vk::raii::PhysicalDevice &physical_device,
                     const vk::raii::CommandPool &command_pool,
                     const vk::raii::Queue &graphics_queue,
                     const char *texture_path);

void write_image_to_png(const vk::raii::Device &device,
                        const vk::raii::PhysicalDevice &physical_device,
                        const vk::raii::CommandPool &command_pool,
                        const vk::raii::Queue &graphics_queue,
                        vk::Image image,
                        std::uint32_t width,
                        std::uint32_t height,
                        vk::ImageLayout layout,
                        vk::PipelineStageFlags stage,
                        vk::AccessFlags access,
                        const char *path);

[[nodiscard]] vk::raii::DescriptorPool
create_descriptor_pool(const vk::raii::Device &device);

[[nodiscard]] std::vector<vk::DescriptorSet>
create_descriptor_sets(const vk::raii::Device &device,
                       vk::DescriptorSetLayout descriptor_set_layout,
                       vk::DescriptorPool descriptor_pool,
                       vk::Sampler sampler,
                       vk::ImageView texture_image_view,
                       const std::vector<Vulkan_buffer> &uniform_buffers,
                       vk::DeviceSize uniform_buffer_size);

[[nodiscard]] vk::DescriptorSet
create_offscreen_descriptor_set(const vk::raii::Device &device,
                                vk::DescriptorSetLayout descriptor_set_layout,
                                vk::DescriptorPool descriptor_pool,
                                vk::Sampler sampler,
                                vk::ImageView texture_image_view,
                                const Vulkan_buffer &uniform_buffer,
                                vk::DeviceSize uniform_buffer_size);

[[nodiscard]] Vulkan_buffer
create_vertex_buffer(const vk::raii::Device &device,
                     const vk::raii::PhysicalDevice &physical_device,
                     const vk::raii::CommandPool &command_pool,
                     const vk::raii::Queue &graphics_queue,
                     const void *vertex_data,
                     vk::DeviceSize vertex_buffer_size);

[[nodiscard]] Vulkan_buffer
create_index_buffer(const vk::raii::Device &device,
                    const vk::raii::PhysicalDevice &physical_device,
                    const vk::raii::CommandPool &command_pool,
                    const vk::raii::Queue &graphics_queue,
                    const std::vector<std::uint16_t> &indices);

[[nodiscard]] Vulkan_buffer
create_uniform_buffer(const vk::raii::Device &device,
                      const vk::raii::PhysicalDevice &physical_device,
                      vk::DeviceSize uniform_buffer_size);

// TODO: this really should just be a std::array
[[nodiscard]] std::vector<Vulkan_buffer>
create_uniform_buffers(const vk::raii::Device &device,
                       const vk::raii::PhysicalDevice &physical_device,
                       vk::DeviceSize uniform_buffer_size);

[[nodiscard]] vk::raii::CommandBuffer
create_draw_command_buffer(const vk::raii::Device &device,
                           const vk::raii::CommandPool &command_pool);

[[nodiscard]] vk::raii::CommandBuffers
create_draw_command_buffers(const vk::raii::Device &device,
                            const vk::raii::CommandPool &command_pool);

#endif // VULKAN_UTILS_HPP
