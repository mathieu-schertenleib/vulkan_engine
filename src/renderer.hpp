#ifndef RENDERER_HPP
#define RENDERER_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_SETTERS
#define VULKAN_HPP_NO_UNION_CONSTRUCTORS
#define VULKAN_HPP_NO_UNION_SETTERS
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

struct Image
{
    vk::raii::Image image;
    vk::raii::ImageView view;
    vk::raii::DeviceMemory memory;
};

struct Buffer
{
    vk::raii::Buffer buffer;
    vk::raii::DeviceMemory memory;
};

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
};

struct Sync_objects
{
    vk::raii::Semaphore image_available_semaphore;
    vk::raii::Semaphore render_finished_semaphore;
    vk::raii::Fence in_flight_fence;
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

    void draw_frame();

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

    [[nodiscard]] vk::raii::CommandPool create_command_pool();

    [[nodiscard]] vk::raii::Sampler create_sampler();

    [[nodiscard]] Image create_texture_image();

    [[nodiscard]] vk::raii::CommandBuffer begin_single_time_commands();

    void
    end_single_time_commands(const vk::raii::CommandBuffer &command_buffer);

    [[nodiscard]] Buffer create_buffer(vk::DeviceSize size,
                                       vk::BufferUsageFlags usage,
                                       vk::MemoryPropertyFlags properties);

    [[nodiscard]] Image create_image(std::uint32_t width,
                                     std::uint32_t height,
                                     vk::Format format,
                                     vk::ImageTiling tiling,
                                     vk::ImageUsageFlags usage,
                                     vk::MemoryPropertyFlags properties);

    void copy_buffer(const vk::Buffer &src,
                     const vk::Buffer &dst,
                     vk::DeviceSize size);

    void copy_buffer_to_image(const vk::Buffer &buffer,
                              const vk::Image &image,
                              std::uint32_t width,
                              std::uint32_t height);

    void transition_image_layout(const vk::Image &image,
                                 vk::ImageLayout old_layout,
                                 vk::ImageLayout new_layout);

    [[nodiscard]] std::uint32_t
    find_memory_type(std::uint32_t type_filter,
                     vk::MemoryPropertyFlags requested_properties);

    [[nodiscard]] vk::raii::DescriptorPool create_descriptor_pool();

    [[nodiscard]] std::vector<vk::DescriptorSet> create_descriptor_sets();

    [[nodiscard]] Buffer create_vertex_buffer();

    [[nodiscard]] Buffer create_index_buffer();

    [[nodiscard]] std::vector<Buffer> create_uniform_buffers();

    [[nodiscard]] vk::raii::CommandBuffers create_command_buffers();

    void record_command_buffer(const vk::raii::CommandBuffer &command_buffer,
                               std::uint32_t image_index);

    [[nodiscard]] std::vector<Sync_objects> create_sync_objects();

    void update_uniform_buffer(std::uint32_t current_image);

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
    std::vector<Sync_objects> m_sync_objects;
    std::uint32_t m_current_frame {};
    bool m_framebuffer_resized {};
    std::uint32_t m_framebuffer_width;
    std::uint32_t m_framebuffer_height;
};

#endif // RENDERER_HPP
