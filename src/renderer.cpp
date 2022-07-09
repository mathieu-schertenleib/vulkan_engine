#include "renderer.hpp"

#include "utils.hpp"

#ifdef ENABLE_DEBUG_UI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include "imgui_impl_vulkan.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <iostream>
#include <limits>
#include <stdexcept>
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

constexpr vk::DebugUtilsMessengerCreateInfoEXT
    g_debug_utils_messenger_create_info {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &debug_callback};

constexpr auto g_khronos_validation_layer = "VK_LAYER_KHRONOS_validation";

[[nodiscard]] bool
khronos_validation_layer_supported(const vk::raii::Context &context)
{
    const auto available_layers = context.enumerateInstanceLayerProperties();

    return std::any_of(available_layers.begin(),
                       available_layers.end(),
                       [](const vk::LayerProperties &layer_properties)
                       {
                           return std::strcmp(layer_properties.layerName,
                                              g_khronos_validation_layer) == 0;
                       });
}

#endif

constexpr std::uint32_t g_max_frames_in_flight {2};

constexpr vk::VertexInputBindingDescription g_vertex_input_binding_description {
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = vk::VertexInputRate::eVertex};

constexpr std::array g_vertex_input_attribute_descriptions {
    vk::VertexInputAttributeDescription {.location = 0,
                                         .binding = 0,
                                         .format = vk::Format::eR32G32B32Sfloat,
                                         .offset = offsetof(Vertex, pos)},
    vk::VertexInputAttributeDescription {.location = 1,
                                         .binding = 0,
                                         .format = vk::Format::eR32G32Sfloat,
                                         .offset =
                                             offsetof(Vertex, tex_coord)}};

constexpr Vertex g_fullscreen_quad_vertices[] {
    {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}}};

constexpr std::uint16_t g_quad_indices[] {0, 1, 2, 2, 3, 0};

constexpr auto g_offscreen_vertex_shader_path =
    "shaders/spv/offscreen.vert.spv";
constexpr auto g_offscreen_fragment_shader_path =
    "shaders/spv/offscreen.frag.spv";
constexpr auto g_final_vertex_shader_path = "shaders/spv/final.vert.spv";
constexpr auto g_final_fragment_shader_path = "shaders/spv/final.frag.spv";
constexpr auto g_texture_path = "assets/texture.jpg";

[[nodiscard]] bool instance_extensions_supported(
    const vk::raii::Context &context,
    const std::vector<const char *> &required_extensions)
{
    const auto available_extensions =
        context.enumerateInstanceExtensionProperties();

    const auto is_extension_supported = [&](const char *extension)
    {
        return std::any_of(
            available_extensions.begin(),
            available_extensions.end(),
            [&](const vk::ExtensionProperties &extension_properties) {
                return std::strcmp(extension_properties.extensionName,
                                   extension) == 0;
            });
    };

    return std::all_of(required_extensions.begin(),
                       required_extensions.end(),
                       [&](const char *extension)
                       { return is_extension_supported(extension); });
}

[[nodiscard]] bool
swapchain_extension_supported(const vk::raii::PhysicalDevice &physical_device)
{
    const auto available_extensions =
        physical_device.enumerateDeviceExtensionProperties();

    return std::any_of(available_extensions.begin(),
                       available_extensions.end(),
                       [](const vk::ExtensionProperties &extension_properties)
                       {
                           return std::strcmp(
                                      extension_properties.extensionName,
                                      VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
                       });
}

[[nodiscard]] std::optional<Queue_family_indices>
get_queue_family_indices(const vk::raii::PhysicalDevice &physical_device,
                         vk::SurfaceKHR surface)
{
    const auto queue_family_properties =
        physical_device.getQueueFamilyProperties();

    std::optional<std::uint32_t> graphics_queue_family_index;
    std::optional<std::uint32_t> present_queue_family_index;

    for (std::uint32_t i {}; const auto &queue_family : queue_family_properties)
    {
        if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            graphics_queue_family_index = i;
        }

        if (physical_device.getSurfaceSupportKHR(i, surface))
        {
            present_queue_family_index = i;
        }

        if (graphics_queue_family_index.has_value() &&
            present_queue_family_index.has_value())
        {
            return Queue_family_indices {graphics_queue_family_index.value(),
                                         present_queue_family_index.value()};
        }

        ++i;
    }

    return std::nullopt;
}

[[nodiscard]] bool
is_physical_device_suitable(const vk::raii::PhysicalDevice &physical_device,
                            vk::SurfaceKHR surface)
{
    if (!swapchain_extension_supported(physical_device))
    {
        return false;
    }

    if (physical_device.getSurfaceFormatsKHR(surface).empty())
    {
        return false;
    }

    if (physical_device.getSurfacePresentModesKHR(surface).empty())
    {
        return false;
    }

    if (!get_queue_family_indices(physical_device, surface).has_value())
    {
        return false;
    }

    return true;
}

[[nodiscard]] vk::raii::Instance
create_instance(const vk::raii::Context &context)
{
    constexpr vk::ApplicationInfo application_info {
        .pApplicationName = "Vulkan Experiment",
        .applicationVersion = {},
        .pEngineName = "Custom Engine",
        .engineVersion = {},
        .apiVersion = VK_API_VERSION_1_3};

    std::uint32_t extension_count {};
    const auto extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    std::vector<const char *> required_extensions(extensions,
                                                  extensions + extension_count);

#ifdef ENABLE_VALIDATION_LAYERS

    if (!khronos_validation_layer_supported(context))
    {
        throw std::runtime_error(std::string("Validation layer ") +
                                 std::string(g_khronos_validation_layer) +
                                 std::string(" is not supported"));
    }

    required_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (!instance_extensions_supported(context, required_extensions))
    {
        throw std::runtime_error("Unsupported instance extension");
    }

    const vk::InstanceCreateInfo instance_create_info {
        .pNext = &g_debug_utils_messenger_create_info,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = &g_khronos_validation_layer,
        .enabledExtensionCount =
            static_cast<std::uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()};

    return {{}, instance_create_info};

#else

    if (!instance_extensions_supported(context, required_extensions))
    {
        throw std::runtime_error("Unsupported instance extension");
    }

    const vk::InstanceCreateInfo instance_create_info {
        .pApplicationInfo = &application_info,
        .enabledExtensionCount =
            static_cast<std::uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()};

    return {{}, instance_create_info};

#endif
}

#ifdef ENABLE_VALIDATION_LAYERS
[[nodiscard]] vk::raii::DebugUtilsMessengerEXT
create_debug_utils_messenger(const vk::raii::Instance &instance)
{
    return {instance, g_debug_utils_messenger_create_info};
}
#endif

[[nodiscard]] vk::raii::SurfaceKHR
create_surface(const vk::raii::Instance &instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create surface");
    }
    return {instance, surface};
}

[[nodiscard]] vk::raii::PhysicalDevice
select_physical_device(const vk::raii::Instance &instance,
                       vk::SurfaceKHR surface)
{
    const auto physical_devices = instance.enumeratePhysicalDevices();
    if (physical_devices.empty())
    {
        throw std::runtime_error("Failed to find a device with Vulkan support");
    }

    std::vector<vk::raii::PhysicalDevice> suitable_devices;
    for (const auto &physical_device : physical_devices)
    {
        if (is_physical_device_suitable(physical_device, surface))
        {
            suitable_devices.push_back(physical_device);
        }
    }

    if (suitable_devices.empty())
    {
        throw std::runtime_error("Failed to find a suitable device");
    }

    const auto it =
        std::find_if(suitable_devices.begin(),
                     suitable_devices.end(),
                     [](const auto &physical_device)
                     {
                         return physical_device.getProperties().deviceType ==
                                vk::PhysicalDeviceType::eDiscreteGpu;
                     });
    if (it != suitable_devices.end())
    {
        // Return the first suitable discrete GPU, if there is one
        return *it;
    }

    // Else return the first suitable device
    return suitable_devices.front();
}

[[nodiscard]] vk::raii::Device
create_device(const vk::raii::PhysicalDevice &physical_device,
              const Queue_family_indices &queue_family_indices)
{
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

    constexpr float queue_priorities[] {1.0f};

    const vk::DeviceQueueCreateInfo graphics_queue_create_info {
        .queueFamilyIndex = queue_family_indices.graphics,
        .queueCount = 1,
        .pQueuePriorities = queue_priorities};
    queue_create_infos.push_back(graphics_queue_create_info);

    if (queue_family_indices.graphics != queue_family_indices.present)
    {
        const vk::DeviceQueueCreateInfo present_queue_create_info {
            .queueFamilyIndex = queue_family_indices.present,
            .queueCount = 1,
            .pQueuePriorities = queue_priorities};
        queue_create_infos.push_back(present_queue_create_info);
    }

    constexpr auto swapchain_extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    const vk::DeviceCreateInfo device_create_info {
        .queueCreateInfoCount =
            static_cast<std::uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &swapchain_extension};

    // NOTE: set pEnabledFeatures if some specific device features are enabled

    return {physical_device, device_create_info};
}

[[nodiscard]] Vulkan_swapchain
create_swapchain(const vk::raii::Device &device,
                 const vk::raii::PhysicalDevice &physical_device,
                 vk::SurfaceKHR surface,
                 const Queue_family_indices &queue_family_indices,
                 std::uint32_t width,
                 std::uint32_t height)
{
    const auto surface_formats = physical_device.getSurfaceFormatsKHR(surface);
    const auto surface_format_it = std::find_if(
        surface_formats.begin(),
        surface_formats.end(),
        [](const vk::SurfaceFormatKHR &format)
        {
            return format.format == vk::Format::eB8G8R8A8Unorm &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    const auto surface_format = (surface_format_it != surface_formats.end())
                                    ? *surface_format_it
                                    : surface_formats.front();

    const auto surface_capabilities =
        physical_device.getSurfaceCapabilitiesKHR(surface);
    vk::Extent2D extent;
    if (surface_capabilities.currentExtent.width !=
        std::numeric_limits<std::uint32_t>::max())
    {
        extent = surface_capabilities.currentExtent;
    }
    else
    {
        extent = vk::Extent2D {width, height};
    }
    extent.width = std::clamp(extent.width,
                              surface_capabilities.minImageExtent.width,
                              surface_capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height,
                               surface_capabilities.minImageExtent.height,
                               surface_capabilities.maxImageExtent.height);

    std::uint32_t min_image_count {3}; // Triple buffering
    if (min_image_count < surface_capabilities.minImageCount)
    {
        min_image_count = surface_capabilities.minImageCount;
    }
    if (surface_capabilities.maxImageCount > 0 &&
        min_image_count > surface_capabilities.maxImageCount)
    {
        min_image_count = surface_capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_create_info {
        .surface = surface,
        .minImageCount = min_image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
        .clipped = VK_TRUE};

    const std::uint32_t queue_family_indices_array[] {
        queue_family_indices.graphics, queue_family_indices.present};
    if (queue_family_indices.graphics != queue_family_indices.present)
    {
        swapchain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices_array;
    }
    else
    {
        swapchain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
        swapchain_create_info.queueFamilyIndexCount = 1;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices_array;
    }

    return Vulkan_swapchain {
        .swapchain = vk::raii::SwapchainKHR(device, swapchain_create_info),
        .format = surface_format.format,
        .extent = extent,
        .min_image_count = min_image_count};
}

[[nodiscard]] std::vector<vk::Image>
get_swapchain_images(const vk::raii::SwapchainKHR &swapchain)
{
    const auto images = swapchain.getImages();
    std::vector<vk::Image> result;
    result.reserve(images.size());

    for (const auto &image : images)
    {
        result.emplace_back(image);
    }

    return result;
}

[[nodiscard]] vk::raii::ImageView create_image_view(
    const vk::raii::Device &device, vk::Image image, vk::Format format)
{
    const vk::ImageViewCreateInfo create_info {
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    return {device, create_info};
}

[[nodiscard]] std::vector<vk::raii::ImageView>
create_swapchain_image_views(const vk::raii::Device &device,
                             const std::vector<vk::Image> &swapchain_images,
                             vk::Format swapchain_format)
{
    std::vector<vk::raii::ImageView> swapchain_image_views;
    swapchain_image_views.reserve(swapchain_images.size());

    for (const auto &swapchain_image : swapchain_images)
    {
        swapchain_image_views.push_back(
            create_image_view(device, swapchain_image, swapchain_format));
    }

    return swapchain_image_views;
}

[[nodiscard]] vk::raii::CommandPool
create_command_pool(const vk::raii::Device &device,
                    std::uint32_t graphics_family_index)
{
    const vk::CommandPoolCreateInfo create_info {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphics_family_index};

    return {device, create_info};
}

[[nodiscard]] vk::raii::CommandBuffer
begin_one_time_submit_command_buffer(const vk::raii::Device &device,
                                     const vk::raii::CommandPool &command_pool)
{
    const vk::CommandBufferAllocateInfo allocate_info {
        .commandPool = *command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};

    vk::raii::CommandBuffers command_buffers(device, allocate_info);
    auto command_buffer = std::move(command_buffers.front());

    constexpr vk::CommandBufferBeginInfo begin_info {
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

    command_buffer.begin(begin_info);

    return command_buffer;
}

void end_one_time_submit_command_buffer(
    const vk::raii::CommandBuffer &command_buffer,
    const vk::raii::Queue &graphics_queue)
{
    command_buffer.end();

    const vk::SubmitInfo submit_info {.commandBufferCount = 1,
                                      .pCommandBuffers = &*command_buffer};

    graphics_queue.submit(submit_info);
    graphics_queue.waitIdle();
}

[[nodiscard]] std::uint32_t
find_memory_type(const vk::raii::PhysicalDevice &physical_device,
                 std::uint32_t type_filter,
                 vk::MemoryPropertyFlags requested_properties)
{
    const auto memory_properties = physical_device.getMemoryProperties();

    for (std::uint32_t i {}; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags &
             requested_properties) == requested_properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find a suitable memory type");
}

[[nodiscard]] Vulkan_buffer
create_buffer(const vk::raii::Device &device,
              const vk::raii::PhysicalDevice &physical_device,
              vk::DeviceSize size,
              vk::BufferUsageFlags usage,
              vk::MemoryPropertyFlags properties)
{
    const vk::BufferCreateInfo buffer_create_info {
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive};

    vk::raii::Buffer buffer(device, buffer_create_info);

    const auto memory_requirements = buffer.getMemoryRequirements();

    const vk::MemoryAllocateInfo allocate_info {
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type(
            physical_device, memory_requirements.memoryTypeBits, properties)};

    vk::raii::DeviceMemory memory(device, allocate_info);

    buffer.bindMemory(*memory, 0);

    return {std::move(buffer), std::move(memory)};
}

[[nodiscard]] Vulkan_image
create_image(const vk::raii::Device &device,
             const vk::raii::PhysicalDevice &physical_device,
             std::uint32_t width,
             std::uint32_t height,
             vk::Format format,
             vk::ImageUsageFlags usage,
             vk::MemoryPropertyFlags properties)
{
    const vk::ImageCreateInfo image_create_info {
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined};

    vk::raii::Image image(device, image_create_info);

    const auto memory_requirements = image.getMemoryRequirements();

    const vk::MemoryAllocateInfo allocate_info {
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type(
            physical_device, memory_requirements.memoryTypeBits, properties)};

    vk::raii::DeviceMemory memory(device, allocate_info);

    image.bindMemory(*memory, 0);

    auto view = create_image_view(device, *image, format);

    return {std::move(image), std::move(view), std::move(memory)};
}

void command_copy_buffer(const vk::raii::CommandBuffer &command_buffer,
                         vk::Buffer src,
                         vk::Buffer dst,
                         vk::DeviceSize size)
{
    const vk::BufferCopy region {.size = size};
    command_buffer.copyBuffer(src, dst, region);
}

void command_copy_buffer_to_image(const vk::raii::CommandBuffer &command_buffer,
                                  vk::Buffer buffer,
                                  vk::Image image,
                                  std::uint32_t width,
                                  std::uint32_t height)
{
    const vk::BufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .mipLevel = 0,
                             .baseArrayLayer = 0,
                             .layerCount = 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}};

    command_buffer.copyBufferToImage(
        buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

void command_copy_image_to_buffer(const vk::raii::CommandBuffer &command_buffer,
                                  vk::Image image,
                                  vk::Buffer buffer,
                                  std::uint32_t width,
                                  std::uint32_t height)
{
    const vk::BufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .mipLevel = 0,
                             .baseArrayLayer = 0,
                             .layerCount = 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}};

    command_buffer.copyImageToBuffer(
        image, vk::ImageLayout::eTransferSrcOptimal, buffer, region);
}

void command_transition_image_layout(
    const vk::raii::CommandBuffer &command_buffer,
    vk::Image image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::PipelineStageFlags src_stage_mask,
    vk::PipelineStageFlags dst_stage_mask,
    vk::AccessFlags src_access_mask,
    vk::AccessFlags dst_access_mask)
{
    const vk::ImageMemoryBarrier memory_barrier {
        .srcAccessMask = src_access_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    command_buffer.pipelineBarrier(
        src_stage_mask, dst_stage_mask, {}, {}, {}, memory_barrier);
}

[[nodiscard]] vk::raii::RenderPass
create_render_pass(const vk::raii::Device &device,
                   vk::Format color_attachment_format)
{
    const vk::AttachmentDescription color_attachment_description {
        .format = color_attachment_format,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR};

    constexpr vk::AttachmentReference color_attachment_reference {
        .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

    const vk::SubpassDescription subpass_description {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference};

    constexpr vk::SubpassDependency subpass_dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = {},
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite};

    const vk::RenderPassCreateInfo create_info {
        .attachmentCount = 1,
        .pAttachments = &color_attachment_description,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = 1,
        .pDependencies = &subpass_dependency};

    return {device, create_info};
}

[[nodiscard]] vk::raii::RenderPass
create_offscreen_render_pass(const vk::raii::Device &device,
                             vk::Format color_attachment_format)
{
    const vk::AttachmentDescription color_attachment_description {
        .format = color_attachment_format,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    constexpr vk::AttachmentReference color_attachment_reference {
        .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal};

    const vk::SubpassDescription subpass_description {
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_reference};

    // Use subpass dependencies for layout transitions
    constexpr vk::SubpassDependency subpass_dependencies[] {
        {.srcSubpass = VK_SUBPASS_EXTERNAL,
         .dstSubpass = 0,
         .srcStageMask = vk::PipelineStageFlagBits::eFragmentShader,
         .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
         .srcAccessMask = vk::AccessFlagBits::eShaderRead,
         .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
         .dependencyFlags = vk::DependencyFlagBits::eByRegion},
        {.srcSubpass = 0,
         .dstSubpass = VK_SUBPASS_EXTERNAL,
         .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
         .dstStageMask = vk::PipelineStageFlagBits::eFragmentShader,
         .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
         .dstAccessMask = vk::AccessFlagBits::eShaderRead,
         .dependencyFlags = vk::DependencyFlagBits::eByRegion}};

    const vk::RenderPassCreateInfo create_info {
        .attachmentCount = 1,
        .pAttachments = &color_attachment_description,
        .subpassCount = 1,
        .pSubpasses = &subpass_description,
        .dependencyCount = std::size(subpass_dependencies),
        .pDependencies = subpass_dependencies};

    return {device, create_info};
}

[[nodiscard]] vk::raii::DescriptorSetLayout
create_offscreen_descriptor_set_layout(const vk::raii::Device &device)
{
    constexpr vk::DescriptorSetLayoutBinding ubo_layout_binding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex};

    constexpr vk::DescriptorSetLayoutBinding sampler_layout_binding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment};

    const vk::DescriptorSetLayoutBinding bindings[] {ubo_layout_binding,
                                                     sampler_layout_binding};

    const vk::DescriptorSetLayoutCreateInfo create_info {
        .bindingCount = std::size(bindings), .pBindings = bindings};

    return {device, create_info};
}

[[nodiscard]] vk::raii::DescriptorSetLayout
create_descriptor_set_layout(const vk::raii::Device &device)
{
    constexpr vk::DescriptorSetLayoutBinding sampler_layout_binding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment};

    const vk::DescriptorSetLayoutCreateInfo create_info {
        .bindingCount = 1, .pBindings = &sampler_layout_binding};

    return {device, create_info};
}

[[nodiscard]] vk::raii::PipelineLayout create_pipeline_layout(
    const vk::raii::Device &device,
    const vk::raii::DescriptorSetLayout &descriptor_set_layout)
{
    const vk::PipelineLayoutCreateInfo create_info {
        .setLayoutCount = 1, .pSetLayouts = &*descriptor_set_layout};

    return {device, create_info};
}

[[nodiscard]] vk::raii::ShaderModule
create_shader_module(const vk::raii::Device &device,
                     const std::vector<std::uint8_t> &shader_code)
{
    const vk::ShaderModuleCreateInfo create_info {
        .codeSize = shader_code.size(),
        .pCode = reinterpret_cast<const std::uint32_t *>(shader_code.data())};

    return {device, create_info};
}

[[nodiscard]] vk::raii::Pipeline create_pipeline(
    const vk::raii::Device &device,
    const char *vertex_shader_path,
    const char *fragment_shader_path,
    vk::Offset2D viewport_offset,
    vk::Extent2D viewport_extent,
    vk::Extent2D framebuffer_extent,
    vk::PipelineLayout pipeline_layout,
    vk::RenderPass render_pass,
    const vk::VertexInputBindingDescription &vertex_binding_description,
    const vk::VertexInputAttributeDescription *vertex_attribute_descriptions,
    std::uint32_t num_vertex_attribute_descriptions)
{
    const auto vertex_shader_code = load_binary_file(vertex_shader_path);
    if (vertex_shader_code.empty())
    {
        throw std::runtime_error(std::string("Failed to load shader \"") +
                                 std::string(vertex_shader_path) +
                                 std::string("\""));
    }
    const auto fragment_shader_code = load_binary_file(fragment_shader_path);
    if (fragment_shader_code.empty())
    {
        throw std::runtime_error(std::string("Failed to load shader \"") +
                                 std::string(fragment_shader_path) +
                                 std::string("\""));
    }

    const auto vertex_shader_module =
        create_shader_module(device, vertex_shader_code);
    const auto fragment_shader_module =
        create_shader_module(device, fragment_shader_code);

    const vk::PipelineShaderStageCreateInfo vertex_shader_stage_create_info {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *vertex_shader_module,
        .pName = "main"};

    const vk::PipelineShaderStageCreateInfo fragment_shader_stage_create_info {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *fragment_shader_module,
        .pName = "main"};

    const vk::PipelineShaderStageCreateInfo shader_stage_create_infos[] {
        vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    const vk::PipelineVertexInputStateCreateInfo
        vertex_input_state_create_info {
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_binding_description,
            .vertexAttributeDescriptionCount =
                num_vertex_attribute_descriptions,
            .pVertexAttributeDescriptions = vertex_attribute_descriptions};

    const vk::PipelineInputAssemblyStateCreateInfo
        input_assembly_state_create_info {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = VK_FALSE};

    const vk::Viewport viewport {
        .x = static_cast<float>(viewport_offset.x),
        .y = static_cast<float>(viewport_offset.y),
        .width = static_cast<float>(viewport_extent.width),
        .height = static_cast<float>(viewport_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f};

    const vk::Rect2D scissor {.offset = {0, 0}, .extent = framebuffer_extent};

    const vk::PipelineViewportStateCreateInfo viewport_state_create_info {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor};

    constexpr vk::PipelineRasterizationStateCreateInfo
        rasterization_state_create_info {
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f};

    constexpr vk::PipelineMultisampleStateCreateInfo
        multisample_state_create_info {.rasterizationSamples =
                                           vk::SampleCountFlagBits::e1,
                                       .sampleShadingEnable = VK_FALSE};

    constexpr vk::PipelineColorBlendAttachmentState
        color_blend_attachment_state {.blendEnable = VK_FALSE,
                                      .colorWriteMask =
                                          vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA};

    const vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info {
        .logicOpEnable = VK_FALSE,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {{0.0f, 0.0f, 0.0f, 0.0f}}};

    const vk::GraphicsPipelineCreateInfo pipeline_create_info {
        .stageCount = 2,
        .pStages = shader_stage_create_infos,
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pColorBlendState = &color_blend_state_create_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0};

    return {device, VK_NULL_HANDLE, pipeline_create_info};
}

[[nodiscard]] vk::raii::Pipeline create_offscreen_pipeline(
    const vk::raii::Device &device,
    const char *vertex_shader_path,
    const char *fragment_shader_path,
    const vk::Extent2D &extent,
    vk::PipelineLayout pipeline_layout,
    vk::RenderPass render_pass,
    const vk::VertexInputBindingDescription &vertex_binding_description,
    const vk::VertexInputAttributeDescription *vertex_attribute_descriptions,
    std::uint32_t num_vertex_attribute_descriptions)
{
    const auto vertex_shader_code = load_binary_file(vertex_shader_path);
    if (vertex_shader_code.empty())
    {
        throw std::runtime_error(std::string("Failed to load shader \"") +
                                 std::string(vertex_shader_path) +
                                 std::string("\""));
    }
    const auto fragment_shader_code = load_binary_file(fragment_shader_path);
    if (fragment_shader_code.empty())
    {
        throw std::runtime_error(std::string("Failed to load shader \"") +
                                 std::string(fragment_shader_path) +
                                 std::string("\""));
    }

    const auto vertex_shader_module =
        create_shader_module(device, vertex_shader_code);
    const auto fragment_shader_module =
        create_shader_module(device, fragment_shader_code);

    const vk::PipelineShaderStageCreateInfo vertex_shader_stage_create_info {
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *vertex_shader_module,
        .pName = "main"};

    const vk::PipelineShaderStageCreateInfo fragment_shader_stage_create_info {
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *fragment_shader_module,
        .pName = "main"};

    const vk::PipelineShaderStageCreateInfo shader_stage_create_infos[] {
        vertex_shader_stage_create_info, fragment_shader_stage_create_info};

    const vk::PipelineVertexInputStateCreateInfo
        vertex_input_state_create_info {
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_binding_description,
            .vertexAttributeDescriptionCount =
                num_vertex_attribute_descriptions,
            .pVertexAttributeDescriptions = vertex_attribute_descriptions};

    const vk::PipelineInputAssemblyStateCreateInfo
        input_assembly_state_create_info {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = VK_FALSE};

    const vk::Viewport viewport {.x = 0.0f,
                                 .y = 0.0f,
                                 .width = static_cast<float>(extent.width),
                                 .height = static_cast<float>(extent.height),
                                 .minDepth = 0.0f,
                                 .maxDepth = 1.0f};

    const vk::Rect2D scissor {.offset = {0, 0}, .extent = extent};

    const vk::PipelineViewportStateCreateInfo viewport_state_create_info {
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor};

    constexpr vk::PipelineRasterizationStateCreateInfo
        rasterization_state_create_info {
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f};

    constexpr vk::PipelineMultisampleStateCreateInfo
        multisample_state_create_info {.rasterizationSamples =
                                           vk::SampleCountFlagBits::e1,
                                       .sampleShadingEnable = VK_FALSE};

    constexpr vk::PipelineColorBlendAttachmentState
        color_blend_attachment_state {.blendEnable = VK_FALSE,
                                      .colorWriteMask =
                                          vk::ColorComponentFlagBits::eR |
                                          vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB |
                                          vk::ColorComponentFlagBits::eA};

    const vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info {
        .logicOpEnable = VK_FALSE,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment_state,
        .blendConstants = {{0.0f, 0.0f, 0.0f, 0.0f}}};

    const vk::GraphicsPipelineCreateInfo pipeline_create_info {
        .stageCount = 2,
        .pStages = shader_stage_create_infos,
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state_create_info,
        .pViewportState = &viewport_state_create_info,
        .pRasterizationState = &rasterization_state_create_info,
        .pMultisampleState = &multisample_state_create_info,
        .pColorBlendState = &color_blend_state_create_info,
        .layout = pipeline_layout,
        .renderPass = render_pass,
        .subpass = 0};

    return {device, VK_NULL_HANDLE, pipeline_create_info};
}

[[nodiscard]] vk::raii::Framebuffer
create_framebuffer(const vk::raii::Device &device,
                   const vk::raii::ImageView &image_view,
                   vk::RenderPass render_pass,
                   std::uint32_t width,
                   std::uint32_t height)
{
    const vk::FramebufferCreateInfo create_info {.renderPass = render_pass,
                                                 .attachmentCount = 1,
                                                 .pAttachments = &*image_view,
                                                 .width = width,
                                                 .height = height,
                                                 .layers = 1};
    return {device, create_info};
}

[[nodiscard]] std::vector<vk::raii::Framebuffer>
create_framebuffers(const vk::raii::Device &device,
                    const std::vector<vk::raii::ImageView> &image_views,
                    vk::RenderPass render_pass,
                    std::uint32_t width,
                    std::uint32_t height)
{
    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(image_views.size());

    for (const auto &image_view : image_views)
    {
        framebuffers.push_back(
            create_framebuffer(device, image_view, render_pass, width, height));
    }

    return framebuffers;
}

[[nodiscard]] vk::raii::Sampler create_sampler(const vk::raii::Device &device)
{
    constexpr vk::SamplerCreateInfo create_info {
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,
        .mipmapMode = vk::SamplerMipmapMode::eNearest,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .anisotropyEnable = VK_FALSE,
        .compareEnable = VK_FALSE,
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        .unnormalizedCoordinates = VK_FALSE};

    return {device, create_info};
}

[[nodiscard]] Vulkan_image
create_texture_image(const vk::raii::Device &device,
                     const vk::raii::PhysicalDevice &physical_device,
                     const vk::raii::CommandPool &command_pool,
                     const vk::raii::Queue &graphics_queue,
                     const char *texture_path)
{
    int width {};
    int height {};
    int channels {};
    auto *const pixels =
        stbi_load(texture_path, &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        throw std::runtime_error(
            std::string("Failed to load texture image \"") +
            std::string(texture_path) + std::string("\""));
    }
    const auto image_size {static_cast<vk::DeviceSize>(width * height * 4)};

    const auto staging_buffer =
        create_buffer(device,
                      physical_device,
                      image_size,
                      vk::BufferUsageFlagBits::eTransferSrc,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent);

    auto *const data = staging_buffer.memory.mapMemory(0, image_size);
    std::memcpy(data, pixels, static_cast<std::size_t>(image_size));
    staging_buffer.memory.unmapMemory();

    stbi_image_free(pixels);

    auto image = create_image(device,
                              physical_device,
                              static_cast<std::uint32_t>(width),
                              static_cast<std::uint32_t>(height),
                              vk::Format::eR8G8B8A8Srgb,
                              vk::ImageUsageFlagBits::eTransferDst |
                                  vk::ImageUsageFlagBits::eSampled,
                              vk::MemoryPropertyFlagBits::eDeviceLocal);

    const auto command_buffer =
        begin_one_time_submit_command_buffer(device, command_pool);

    command_transition_image_layout(command_buffer,
                                    *image.image,
                                    vk::ImageLayout::eUndefined,
                                    vk::ImageLayout::eTransferDstOptimal,
                                    vk::PipelineStageFlagBits::eTopOfPipe,
                                    vk::PipelineStageFlagBits::eTransfer,
                                    {},
                                    vk::AccessFlagBits::eTransferWrite);

    command_copy_buffer_to_image(command_buffer,
                                 *staging_buffer.buffer,
                                 *image.image,
                                 static_cast<std::uint32_t>(width),
                                 static_cast<std::uint32_t>(height));

    command_transition_image_layout(command_buffer,
                                    *image.image,
                                    vk::ImageLayout::eTransferDstOptimal,
                                    vk::ImageLayout::eShaderReadOnlyOptimal,
                                    vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eFragmentShader,
                                    vk::AccessFlagBits::eTransferWrite,
                                    vk::AccessFlagBits::eShaderRead);

    end_one_time_submit_command_buffer(command_buffer, graphics_queue);

    return image;
}

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
                        const char *path)
{
    const auto image_size {static_cast<vk::DeviceSize>(width * height * 4)};

    const auto staging_buffer =
        create_buffer(device,
                      physical_device,
                      image_size,
                      vk::BufferUsageFlagBits::eTransferDst,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent);

    const auto command_buffer =
        begin_one_time_submit_command_buffer(device, command_pool);

    command_transition_image_layout(command_buffer,
                                    image,
                                    layout,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    stage,
                                    vk::PipelineStageFlagBits::eTransfer,
                                    access,
                                    vk::AccessFlagBits::eTransferRead);

    command_copy_image_to_buffer(
        command_buffer, image, *staging_buffer.buffer, width, height);

    command_transition_image_layout(command_buffer,
                                    image,
                                    vk::ImageLayout::eTransferSrcOptimal,
                                    layout,
                                    vk::PipelineStageFlagBits::eTransfer,
                                    stage,
                                    vk::AccessFlagBits::eTransferRead,
                                    access);

    end_one_time_submit_command_buffer(command_buffer, graphics_queue);

    const auto *const data = staging_buffer.memory.mapMemory(0, image_size);

    if (!stbi_write_png(path,
                        static_cast<int>(width),
                        static_cast<int>(height),
                        STBI_rgb_alpha,
                        data,
                        static_cast<int>(width) * STBI_rgb_alpha))
    {
        throw std::runtime_error(std::string("Failed to write image \"") +
                                 std::string(path) + std::string("\""));
    }

    staging_buffer.memory.unmapMemory();
}

[[nodiscard]] vk::raii::DescriptorPool
create_descriptor_pool(const vk::raii::Device &device)
{
    constexpr vk::DescriptorPoolSize pool_sizes[] {
        {vk::DescriptorType::eCombinedImageSampler, g_max_frames_in_flight},
        {vk::DescriptorType::eUniformBuffer, g_max_frames_in_flight}};
    const vk::DescriptorPoolCreateInfo create_info {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = g_max_frames_in_flight * std::size(pool_sizes),
        .poolSizeCount = static_cast<std::uint32_t>(std::size(pool_sizes)),
        .pPoolSizes = pool_sizes};

    return {device, create_info};
}

[[nodiscard]] vk::raii::DescriptorPool
create_imgui_descriptor_pool(const vk::raii::Device &device)
{
    // Copied from imgui/examples/example_glfw_vulkan
    constexpr vk::DescriptorPoolSize pool_sizes[] {
        {vk::DescriptorType::eSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eSampledImage, 1000},
        {vk::DescriptorType::eStorageImage, 1000},
        {vk::DescriptorType::eUniformTexelBuffer, 1000},
        {vk::DescriptorType::eStorageTexelBuffer, 1000},
        {vk::DescriptorType::eUniformBuffer, 1000},
        {vk::DescriptorType::eStorageBuffer, 1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {vk::DescriptorType::eInputAttachment, 1000}};
    const vk::DescriptorPoolCreateInfo create_info {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1000 * std::size(pool_sizes),
        .poolSizeCount = static_cast<std::uint32_t>(std::size(pool_sizes)),
        .pPoolSizes = pool_sizes};

    return {device, create_info};
}

[[nodiscard]] std::vector<vk::DescriptorSet>
create_descriptor_sets(const vk::raii::Device &device,
                       vk::DescriptorSetLayout descriptor_set_layout,
                       vk::DescriptorPool descriptor_pool,
                       vk::Sampler sampler,
                       vk::ImageView texture_image_view)
{
    std::array<vk::DescriptorSetLayout, g_max_frames_in_flight> layouts;
    std::fill(layouts.begin(), layouts.end(), descriptor_set_layout);

    const vk::DescriptorSetAllocateInfo allocate_info {
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = g_max_frames_in_flight,
        .pSetLayouts = layouts.data()};

    std::vector<vk::DescriptorSet> descriptor_sets {
        (*device).allocateDescriptorSets(allocate_info)};

    for (std::size_t i {}; i < g_max_frames_in_flight; ++i)
    {
        const vk::DescriptorImageInfo image_info {
            .sampler = sampler,
            .imageView = texture_image_view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        const std::array descriptor_writes {vk::WriteDescriptorSet {
            .dstSet = descriptor_sets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &image_info}};

        device.updateDescriptorSets(descriptor_writes, {});
    }

    return descriptor_sets;
}

[[nodiscard]] vk::DescriptorSet
create_offscreen_descriptor_set(const vk::raii::Device &device,
                                vk::DescriptorSetLayout descriptor_set_layout,
                                vk::DescriptorPool descriptor_pool,
                                vk::Sampler sampler,
                                vk::ImageView texture_image_view,
                                const Vulkan_buffer &uniform_buffer,
                                vk::DeviceSize uniform_buffer_size)
{
    const vk::DescriptorSetAllocateInfo allocate_info {
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout};

    auto descriptor_set =
        (*device).allocateDescriptorSets(allocate_info).front();

    const vk::DescriptorBufferInfo buffer_info {.buffer =
                                                    *uniform_buffer.buffer,
                                                .offset = 0,
                                                .range = uniform_buffer_size};

    const vk::DescriptorImageInfo image_info {
        .sampler = sampler,
        .imageView = texture_image_view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    const std::array descriptor_writes {
        vk::WriteDescriptorSet {.dstSet = descriptor_set,
                                .dstBinding = 0,
                                .dstArrayElement = 0,
                                .descriptorCount = 1,
                                .descriptorType =
                                    vk::DescriptorType::eUniformBuffer,
                                .pBufferInfo = &buffer_info},
        vk::WriteDescriptorSet {.dstSet = descriptor_set,
                                .dstBinding = 1,
                                .dstArrayElement = 0,
                                .descriptorCount = 1,
                                .descriptorType =
                                    vk::DescriptorType::eCombinedImageSampler,
                                .pImageInfo = &image_info}};

    device.updateDescriptorSets(descriptor_writes, {});

    return descriptor_set;
}

[[nodiscard]] Vulkan_buffer
create_vertex_buffer(const vk::raii::Device &device,
                     const vk::raii::PhysicalDevice &physical_device,
                     const vk::raii::CommandPool &command_pool,
                     const vk::raii::Queue &graphics_queue,
                     const void *vertex_data,
                     vk::DeviceSize vertex_buffer_size)
{
    const auto staging_buffer =
        create_buffer(device,
                      physical_device,
                      vertex_buffer_size,
                      vk::BufferUsageFlagBits::eTransferSrc,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent);

    const auto data = staging_buffer.memory.mapMemory(0, vertex_buffer_size);
    std::memcpy(data, vertex_data, vertex_buffer_size);
    staging_buffer.memory.unmapMemory();

    auto vertex_buffer =
        create_buffer(device,
                      physical_device,
                      vertex_buffer_size,
                      vk::BufferUsageFlagBits::eTransferDst |
                          vk::BufferUsageFlagBits::eVertexBuffer,
                      vk::MemoryPropertyFlagBits::eDeviceLocal);

    const auto command_buffer =
        begin_one_time_submit_command_buffer(device, command_pool);

    command_copy_buffer(command_buffer,
                        *staging_buffer.buffer,
                        *vertex_buffer.buffer,
                        vertex_buffer_size);

    end_one_time_submit_command_buffer(command_buffer, graphics_queue);

    return vertex_buffer;
}

[[nodiscard]] Vulkan_buffer
create_index_buffer(const vk::raii::Device &device,
                    const vk::raii::PhysicalDevice &physical_device,
                    const vk::raii::CommandPool &command_pool,
                    const vk::raii::Queue &graphics_queue,
                    const std::uint16_t *index_data,
                    vk::DeviceSize index_buffer_size)
{
    const auto staging_buffer =
        create_buffer(device,
                      physical_device,
                      index_buffer_size,
                      vk::BufferUsageFlagBits::eTransferSrc,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent);

    const auto data = staging_buffer.memory.mapMemory(0, index_buffer_size);
    std::memcpy(data, index_data, index_buffer_size);
    staging_buffer.memory.unmapMemory();

    auto index_buffer = create_buffer(device,
                                      physical_device,
                                      index_buffer_size,
                                      vk::BufferUsageFlagBits::eTransferDst |
                                          vk::BufferUsageFlagBits::eIndexBuffer,
                                      vk::MemoryPropertyFlagBits::eDeviceLocal);

    const auto command_buffer =
        begin_one_time_submit_command_buffer(device, command_pool);

    command_copy_buffer(command_buffer,
                        *staging_buffer.buffer,
                        *index_buffer.buffer,
                        index_buffer_size);

    end_one_time_submit_command_buffer(command_buffer, graphics_queue);

    return index_buffer;
}

[[nodiscard]] Vulkan_buffer
create_uniform_buffer(const vk::raii::Device &device,
                      const vk::raii::PhysicalDevice &physical_device,
                      vk::DeviceSize uniform_buffer_size)
{
    return create_buffer(device,
                         physical_device,
                         uniform_buffer_size,
                         vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
}

[[nodiscard]] vk::raii::CommandBuffers
create_draw_command_buffers(const vk::raii::Device &device,
                            const vk::raii::CommandPool &command_pool)
{
    const vk::CommandBufferAllocateInfo allocate_info {
        .commandPool = *command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = g_max_frames_in_flight};

    return {device, allocate_info};
}

[[nodiscard]] Vulkan_image create_offscreen_color_attachment(
    const vk::raii::Device &device,
    const vk::raii::PhysicalDevice &physical_device,
    const vk::raii::CommandPool &command_pool,
    const vk::raii::Queue &graphics_queue,
    std::uint32_t width,
    std::uint32_t height,
    vk::Format format)
{
    auto image = create_image(device,
                              physical_device,
                              width,
                              height,
                              format,
                              vk::ImageUsageFlagBits::eColorAttachment |
                                  vk::ImageUsageFlagBits::eSampled,
                              vk::MemoryPropertyFlagBits::eDeviceLocal);

    const auto command_buffer =
        begin_one_time_submit_command_buffer(device, command_pool);

    command_transition_image_layout(
        command_buffer,
        *image.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentWrite);

    end_one_time_submit_command_buffer(command_buffer, graphics_queue);

    return image;
}

struct Uniform_buffer_object
{
    glm::vec2 resolution;
    glm::vec2 mouse_position;
    float time;
};

void check_vk_result(VkResult result)
{
    if (result != VK_SUCCESS)
        throw std::runtime_error(
            "Vulkan error in ImGui: result is " +
            vk::to_string(static_cast<vk::Result>(result)));
}

[[nodiscard]] constexpr std::uint32_t
scaling_factor(std::uint32_t offscreen_width,
               std::uint32_t offscreen_height,
               std::uint32_t swapchain_width,
               std::uint32_t swapchain_height)
{
    const auto horizontal_scaling_factor = swapchain_width / offscreen_width;
    const auto vertical_scaling_factor = swapchain_height / offscreen_height;
    return std::max(
        std::min(horizontal_scaling_factor, vertical_scaling_factor), 1u);
}

[[nodiscard]] constexpr vk::Extent2D
viewport_extent(std::uint32_t offscreen_width,
                std::uint32_t offscreen_height,
                std::uint32_t swapchain_width,
                std::uint32_t swapchain_height)
{
    const auto scale = scaling_factor(
        offscreen_width, offscreen_height, swapchain_width, swapchain_height);
    return {scale * offscreen_width, scale * offscreen_height};
}

[[nodiscard]] constexpr vk::Offset2D
viewport_offset(std::uint32_t offscreen_width,
                std::uint32_t offscreen_height,
                std::uint32_t swapchain_width,
                std::uint32_t swapchain_height)
{
    const auto extent = viewport_extent(
        offscreen_width, offscreen_height, swapchain_width, swapchain_height);
    const auto offset_x = (static_cast<std::int32_t>(swapchain_width) -
                           static_cast<std::int32_t>(extent.width)) /
                          2;
    const auto offset_y = (static_cast<std::int32_t>(swapchain_height) -
                           static_cast<std::int32_t>(extent.height)) /
                          2;
    return {offset_x, offset_y};
}

} // namespace

Renderer::Renderer(GLFWwindow *window,
                   std::uint32_t width,
                   std::uint32_t height)
    : m_context {}
    , m_instance {create_instance(m_context)}
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
                                    height)}
    , m_swapchain_images {get_swapchain_images(m_swapchain.swapchain)}
    , m_swapchain_image_views {create_swapchain_image_views(
          m_device, m_swapchain_images, m_swapchain.format)}
    , m_sampler {create_sampler(m_device)}
    , m_descriptor_pool {create_descriptor_pool(m_device)}
    ,
#ifdef ENABLE_DEBUG_UI
    m_imgui_descriptor_pool {create_imgui_descriptor_pool(m_device)}
    ,
#endif
    m_command_pool {
        create_command_pool(m_device, m_queue_family_indices.graphics)}
    , m_vertex_array {create_vertex_array()}
    , m_offscreen_width {160}
    , m_offscreen_height {90}
    , m_offscreen_color_attachment {create_offscreen_color_attachment(
          m_device,
          m_physical_device,
          m_command_pool,
          m_graphics_queue,
          m_offscreen_width,
          m_offscreen_height,
          m_swapchain.format)}
    , m_offscreen_render_pass {create_offscreen_render_pass(m_device,
                                                            m_swapchain.format)}
    , m_offscreen_descriptor_set_layout {create_offscreen_descriptor_set_layout(
          m_device)}
    , m_offscreen_pipeline_layout {create_pipeline_layout(
          m_device, m_offscreen_descriptor_set_layout)}
    , m_offscreen_pipeline {create_offscreen_pipeline(
          m_device,
          g_offscreen_vertex_shader_path,
          g_offscreen_fragment_shader_path,
          {m_offscreen_width, m_offscreen_height},
          *m_offscreen_pipeline_layout,
          *m_offscreen_render_pass,
          g_vertex_input_binding_description,
          g_vertex_input_attribute_descriptions.data(),
          g_vertex_input_attribute_descriptions.size())}
    , m_offscreen_framebuffer {create_framebuffer(
          m_device,
          m_offscreen_color_attachment.view,
          *m_offscreen_render_pass,
          m_offscreen_width,
          m_offscreen_height)}
    , m_offscreen_texture_image {create_texture_image(m_device,
                                                      m_physical_device,
                                                      m_command_pool,
                                                      m_graphics_queue,
                                                      g_texture_path)}
    , m_offscreen_vertex_buffer {create_vertex_buffer(
          m_device,
          m_physical_device,
          m_command_pool,
          m_graphics_queue,
          m_vertex_array.vertices.data(),
          m_vertex_array.vertices.size() * sizeof(Vertex))}
    , m_offscreen_index_buffer {create_index_buffer(
          m_device,
          m_physical_device,
          m_command_pool,
          m_graphics_queue,
          m_vertex_array.indices.data(),
          m_vertex_array.indices.size() * sizeof(std::uint16_t))}
    , m_offscreen_uniform_buffer {create_uniform_buffer(
          m_device, m_physical_device, sizeof(Uniform_buffer_object))}
    , m_offscreen_descriptor_set {create_offscreen_descriptor_set(
          m_device,
          *m_offscreen_descriptor_set_layout,
          *m_descriptor_pool,
          *m_sampler,
          *m_offscreen_texture_image.view,
          m_offscreen_uniform_buffer,
          sizeof(Uniform_buffer_object))}
    , m_framebuffer_width {width}
    , m_framebuffer_height {height}
    , m_render_pass {create_render_pass(m_device, m_swapchain.format)}
    , m_descriptor_set_layout {create_descriptor_set_layout(m_device)}
    , m_pipeline_layout {create_pipeline_layout(m_device,
                                                m_descriptor_set_layout)}
    , m_pipeline {create_pipeline(m_device,
                                  g_final_vertex_shader_path,
                                  g_final_fragment_shader_path,
                                  viewport_offset(m_offscreen_width,
                                                  m_offscreen_height,
                                                  m_swapchain.extent.width,
                                                  m_swapchain.extent.height),
                                  viewport_extent(m_offscreen_width,
                                                  m_offscreen_height,
                                                  m_swapchain.extent.width,
                                                  m_swapchain.extent.height),
                                  {m_framebuffer_width, m_framebuffer_height},
                                  *m_pipeline_layout,
                                  *m_render_pass,
                                  g_vertex_input_binding_description,
                                  g_vertex_input_attribute_descriptions.data(),
                                  g_vertex_input_attribute_descriptions.size())}
    , m_framebuffers {create_framebuffers(m_device,
                                          m_swapchain_image_views,
                                          *m_render_pass,
                                          m_swapchain.extent.width,
                                          m_swapchain.extent.height)}
    , m_vertex_buffer {create_vertex_buffer(
          m_device,
          m_physical_device,
          m_command_pool,
          m_graphics_queue,
          g_fullscreen_quad_vertices,
          std::size(g_fullscreen_quad_vertices) * sizeof(Vertex))}
    , m_index_buffer {create_index_buffer(m_device,
                                          m_physical_device,
                                          m_command_pool,
                                          m_graphics_queue,
                                          g_quad_indices,
                                          std::size(g_quad_indices) *
                                              sizeof(std::uint16_t))}
    , m_descriptor_sets {create_descriptor_sets(
          m_device,
          *m_descriptor_set_layout,
          *m_descriptor_pool,
          *m_sampler,
          *m_offscreen_color_attachment.view)}
    , m_draw_command_buffers {create_draw_command_buffers(m_device,
                                                          m_command_pool)}
    , m_sync_objects {create_sync_objects()}
{
#ifdef ENABLE_DEBUG_UI
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info {};
    init_info.Instance = *m_instance;
    init_info.PhysicalDevice = *m_physical_device;
    init_info.Device = *m_device;
    init_info.QueueFamily = m_queue_family_indices.graphics;
    init_info.Queue = *m_graphics_queue;
    init_info.DescriptorPool = *m_imgui_descriptor_pool;
    init_info.Subpass = 0;
    init_info.MinImageCount = m_swapchain.min_image_count;
    init_info.ImageCount =
        static_cast<std::uint32_t>(m_swapchain_images.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = &check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, *m_render_pass);

    ImFontConfig font_config {};
    font_config.SizePixels = 32.0f;
    ImGui::GetIO().Fonts->AddFontDefault(&font_config);

    const auto command_buffer =
        begin_one_time_submit_command_buffer(m_device, m_command_pool);
    ImGui_ImplVulkan_CreateFontsTexture(*command_buffer);
    end_one_time_submit_command_buffer(command_buffer, m_graphics_queue);
    m_device.waitIdle();
    ImGui_ImplVulkan_DestroyFontUploadObjects();
#endif
}

Renderer::~Renderer()
{
    m_device.waitIdle();

#ifdef ENABLE_DEBUG_UI
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
#endif
}

Vertex_array Renderer::create_vertex_array()
{
    Vertex_array vertex_array;

    const auto create_quad = [&](const glm::vec2 &position,
                                 const glm::vec2 &size,
                                 const glm::vec2 &tex_coord_0,
                                 const glm::vec2 &tex_coord_1)
    {
        const auto vertex_offset =
            static_cast<std::uint16_t>(vertex_array.vertices.size());
        vertex_array.vertices.push_back(
            {{position.x, position.y, 0.0f}, tex_coord_0});
        vertex_array.vertices.push_back(
            {{position.x, position.y + size.y, 0.0f},
             {tex_coord_0.x, tex_coord_1.y}});
        vertex_array.vertices.push_back(
            {{position.x + size.x, position.y + size.y, 0.0f}, tex_coord_1});
        vertex_array.vertices.push_back(
            {{position.x + size.x, position.y, 0.0f},
             {tex_coord_1.x, tex_coord_0.y}});
        vertex_array.indices.push_back(vertex_offset + 0);
        vertex_array.indices.push_back(vertex_offset + 1);
        vertex_array.indices.push_back(vertex_offset + 2);
        vertex_array.indices.push_back(vertex_offset + 2);
        vertex_array.indices.push_back(vertex_offset + 3);
        vertex_array.indices.push_back(vertex_offset + 0);
    };

    create_quad({-1.0f, -1.0f}, {2.0f, 2.0f}, {0.0f, 0.0f}, {1.0f, 1.0f});

    create_quad({0.0f, 0.0f}, {0.5f, 0.5f}, {0.1f, 0.1f}, {0.2f, 0.2f});

    return vertex_array;
}

Sync_objects Renderer::create_sync_objects()
{
    Sync_objects result;

    for (std::size_t i {}; i < g_max_frames_in_flight; ++i)
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

void Renderer::update_uniform_buffer(float time,
                                     const glm::vec2 &mouse_position) const
{
    const auto [offset_x, offset_y] = viewport_offset(m_offscreen_width,
                                                      m_offscreen_height,
                                                      m_framebuffer_width,
                                                      m_framebuffer_height);
    const auto [extent_x, extent_y] = viewport_extent(m_offscreen_width,
                                                      m_offscreen_height,
                                                      m_framebuffer_width,
                                                      m_framebuffer_height);
    const auto offscreen_mouse_normalized =
        (mouse_position - glm::vec2 {offset_x, offset_y}) /
        glm::vec2 {extent_x, extent_y};
    const auto offscreen_mouse_pos =
        offscreen_mouse_normalized *
        glm::vec2 {m_offscreen_width, m_offscreen_height};

    const Uniform_buffer_object ubo {
        .resolution = {static_cast<float>(m_offscreen_width),
                       static_cast<float>(m_offscreen_height)},
        .mouse_position = offscreen_mouse_pos,
        .time = time};

    auto *const data =
        m_offscreen_uniform_buffer.memory.mapMemory(0, sizeof(ubo));
    std::memcpy(data, &ubo, sizeof(ubo));
    m_offscreen_uniform_buffer.memory.unmapMemory();
}

void Renderer::record_command_buffer(std::uint32_t image_index)
{
    const auto &command_buffer = m_draw_command_buffers[m_current_frame];

    command_buffer.begin({});

    // Offscreen pass
    {
        constexpr vk::ClearValue clear_color_value {
            .color = {{{0.0f, 0.0f, 0.0f, 1.0f}}}};

        const vk::RenderPassBeginInfo render_pass_begin_info {
            .renderPass = *m_offscreen_render_pass,
            .framebuffer = *m_offscreen_framebuffer,
            .renderArea = {.offset = {0, 0},
                           .extent = {m_offscreen_width, m_offscreen_height}},
            .clearValueCount = 1,
            .pClearValues = &clear_color_value};

        command_buffer.beginRenderPass(render_pass_begin_info,
                                       vk::SubpassContents::eInline);

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          *m_offscreen_pipeline_layout,
                                          0,
                                          m_offscreen_descriptor_set,
                                          {});

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                    *m_offscreen_pipeline);

        command_buffer.bindVertexBuffers(
            0, *m_offscreen_vertex_buffer.buffer, {0});

        command_buffer.bindIndexBuffer(
            *m_offscreen_index_buffer.buffer, 0, vk::IndexType::eUint16);

        command_buffer.drawIndexed(
            static_cast<std::uint32_t>(m_vertex_array.indices.size()),
            1,
            0,
            0,
            0);

        command_buffer.endRenderPass();
    }

    // NOTE: explicit synchronization not required between the render passes, as
    // it is done implicitly via subpass dependencies

    // Final pass
    {
        constexpr vk::ClearValue clear_color_value {
            .color = {{{0.0f, 0.0f, 0.0f, 1.0f}}}};

        const vk::RenderPassBeginInfo render_pass_begin_info {
            .renderPass = *m_render_pass,
            .framebuffer = *m_framebuffers[image_index],
            .renderArea = {.offset = {0, 0}, .extent = m_swapchain.extent},
            .clearValueCount = 1,
            .pClearValues = &clear_color_value};

        command_buffer.beginRenderPass(render_pass_begin_info,
                                       vk::SubpassContents::eInline);

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          *m_pipeline_layout,
                                          0,
                                          m_descriptor_sets[m_current_frame],
                                          {});

        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                    *m_pipeline);

        command_buffer.bindVertexBuffers(0, *m_vertex_buffer.buffer, {0});

        command_buffer.bindIndexBuffer(
            *m_index_buffer.buffer, 0, vk::IndexType::eUint16);

        command_buffer.drawIndexed(
            static_cast<std::uint32_t>(std::size(g_quad_indices)), 1, 0, 0, 0);

#ifdef ENABLE_DEBUG_UI
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffer);
#endif

        command_buffer.endRenderPass();
    }

    command_buffer.end();
}

void Renderer::recreate_swapchain()
{
    if (m_framebuffer_width == 0 || m_framebuffer_height == 0)
    {
        return;
    }

    m_device.waitIdle();

    m_swapchain = create_swapchain(m_device,
                                   m_physical_device,
                                   *m_surface,
                                   m_queue_family_indices,
                                   m_framebuffer_width,
                                   m_framebuffer_height);
    m_swapchain_images = get_swapchain_images(m_swapchain.swapchain);
    m_swapchain_image_views = create_swapchain_image_views(
        m_device, m_swapchain_images, m_swapchain.format);
    m_render_pass = create_render_pass(m_device, m_swapchain.format);
    m_pipeline_layout =
        create_pipeline_layout(m_device, m_descriptor_set_layout);
    m_pipeline = create_pipeline(m_device,
                                 g_final_vertex_shader_path,
                                 g_final_fragment_shader_path,
                                 viewport_offset(m_offscreen_width,
                                                 m_offscreen_height,
                                                 m_swapchain.extent.width,
                                                 m_swapchain.extent.height),
                                 viewport_extent(m_offscreen_width,
                                                 m_offscreen_height,
                                                 m_swapchain.extent.width,
                                                 m_swapchain.extent.height),
                                 {m_framebuffer_width, m_framebuffer_height},
                                 *m_pipeline_layout,
                                 *m_render_pass,
                                 g_vertex_input_binding_description,
                                 g_vertex_input_attribute_descriptions.data(),
                                 g_vertex_input_attribute_descriptions.size());
    m_framebuffers = create_framebuffers(m_device,
                                         m_swapchain_image_views,
                                         *m_render_pass,
                                         m_swapchain.extent.width,
                                         m_swapchain.extent.height);
}

void Renderer::resize_framebuffer(std::uint32_t width, std::uint32_t height)
{
    m_framebuffer_resized = true;
    m_framebuffer_width = width;
    m_framebuffer_height = height;
}

void Renderer::draw_frame(float time, const glm::vec2 &mouse_position)
{
#ifdef ENABLE_DEBUG_UI
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::Begin("Debug"))
    {
        ImGui::Text("%.1f fps, %.3f ms/frame",
                    static_cast<double>(ImGui::GetIO().Framerate),
                    1000.0 / static_cast<double>(ImGui::GetIO().Framerate));
        ImGui::Text(
            "Offscreen: %d x %d", m_offscreen_width, m_offscreen_height);
        const auto scale = scaling_factor(m_offscreen_width,
                                          m_offscreen_height,
                                          m_swapchain.extent.width,
                                          m_swapchain.extent.height);
        const auto extent = viewport_extent(m_offscreen_width,
                                            m_offscreen_height,
                                            m_swapchain.extent.width,
                                            m_swapchain.extent.height);
        ImGui::Text("Upscaled: %d x %d (%dx scaling)",
                    extent.width,
                    extent.height,
                    scale);
        ImGui::Text(
            "Framebuffer: %d x %d", m_framebuffer_width, m_framebuffer_height);
    }
    ImGui::End();

    ImGui::Render();
#endif

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

    update_uniform_buffer(time, mouse_position);

    m_device.resetFences(*m_sync_objects.in_flight_fences[m_current_frame]);

    m_draw_command_buffers[m_current_frame].reset();

    record_command_buffer(image_index);

    const vk::PipelineStageFlags wait_stages[] {
        vk::PipelineStageFlagBits::eColorAttachmentOutput};

    const vk::SubmitInfo submit_info {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &*m_sync_objects.image_available_semaphores[m_current_frame],
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &*m_draw_command_buffers[m_current_frame],
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
        recreate_swapchain();
        m_framebuffer_resized = false;
    }
    else if (present_result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Failed to present swapchain image");
    }

    m_current_frame = (m_current_frame + 1) % g_max_frames_in_flight;
}
