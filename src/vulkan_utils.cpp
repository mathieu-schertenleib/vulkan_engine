#include "vulkan_utils.hpp"

#include "utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <limits>
#include <set>
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
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ||
        message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        std::cerr << callback_data->pMessage << std::endl;
    else
        std::cout << callback_data->pMessage << std::endl;

    return VK_FALSE;
}

inline constexpr vk::DebugUtilsMessengerCreateInfoEXT
    debug_utils_messenger_create_info {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                           vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                       vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = debug_callback};

inline constexpr auto khronos_validation_layer = "VK_LAYER_KHRONOS_validation";

[[nodiscard]] bool khronos_validation_layer_supported()
{
    const auto available_layers = vk::enumerateInstanceLayerProperties();

    return std::any_of(available_layers.begin(),
                       available_layers.end(),
                       [](const auto &layer) {
                           return std::strcmp(layer.layerName,
                                              khronos_validation_layer) == 0;
                       });
}

#endif

[[nodiscard]] bool
swapchain_extension_supported(const vk::raii::PhysicalDevice &physical_device)
{
    const auto available_extensions =
        physical_device.enumerateDeviceExtensionProperties();

    return std::any_of(available_extensions.begin(),
                       available_extensions.end(),
                       [](const auto &extension)
                       {
                           return std::strcmp(
                                      extension.extensionName,
                                      VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0;
                       });
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

} // namespace

vk::raii::Instance create_instance()
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

    if (!khronos_validation_layer_supported())
    {
        throw std::runtime_error(std::string("Validation layer ") +
                                 std::string(khronos_validation_layer) +
                                 std::string(" is not supported"));
    }

    auto enabled_extensions = required_extensions;
    enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    const vk::InstanceCreateInfo instance_create_info {
        .pNext = &debug_utils_messenger_create_info,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 1,
        .ppEnabledLayerNames = &khronos_validation_layer,
        .enabledExtensionCount =
            static_cast<std::uint32_t>(enabled_extensions.size()),
        .ppEnabledExtensionNames = enabled_extensions.data()};

    return {{}, instance_create_info};

#else

    const vk::InstanceCreateInfo instance_create_info {
        .pApplicationInfo = &application_info,
        .enabledExtensionCount =
            static_cast<std::uint32_t>(required_extensions.size()),
        .ppEnabledExtensionNames = required_extensions.data()};

    return {{}, instance_create_info};

#endif
}

#ifdef ENABLE_VALIDATION_LAYERS
vk::raii::DebugUtilsMessengerEXT
create_debug_utils_messenger(const vk::raii::Instance &instance)
{
    return {instance, debug_utils_messenger_create_info};
}
#endif

vk::raii::SurfaceKHR create_surface(const vk::raii::Instance &instance,
                                    GLFWwindow *window)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &surface) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create surface");
    }
    return {instance, surface};
}

vk::raii::PhysicalDevice
select_physical_device(const vk::raii::Instance &instance,
                       vk::SurfaceKHR surface)
{
    vk::raii::PhysicalDevices physical_devices(instance);

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
        return *it;
    }

    return suitable_devices.front();
}

std::optional<Queue_family_indices>
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

vk::raii::Device create_device(const vk::raii::PhysicalDevice &physical_device,
                               const Queue_family_indices &queue_family_indices)
{
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

    std::set<std::uint32_t> unique_queue_family_indices {
        queue_family_indices.graphics, queue_family_indices.present};

    for (const auto queue_family_index : unique_queue_family_indices)
    {
        constexpr float queue_priorities[] {1.0f};
        const vk::DeviceQueueCreateInfo queue_create_info {
            .queueFamilyIndex = queue_family_index,
            .queueCount = 1,
            .pQueuePriorities = queue_priorities};
        queue_create_infos.push_back(queue_create_info);
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

Swapchain create_swapchain(const vk::raii::Device &device,
                           const vk::raii::PhysicalDevice &physical_device,
                           vk::SurfaceKHR surface,
                           const Queue_family_indices &queue_family_indices,
                           std::uint32_t width,
                           std::uint32_t height,
                           bool fastest_present_mode)
{
    const auto surface_formats = physical_device.getSurfaceFormatsKHR(surface);
    const auto surface_format_it = std::find_if(
        surface_formats.begin(),
        surface_formats.end(),
        [](const auto &format)
        {
            return format.format == vk::Format::eB8G8R8A8Unorm &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    const auto surface_format = (surface_format_it != surface_formats.end())
                                    ? *surface_format_it
                                    : surface_formats.front();

    auto present_mode = vk::PresentModeKHR::eFifo;
    if (fastest_present_mode)
    {
        const auto present_modes =
            physical_device.getSurfacePresentModesKHR(surface);
        const auto present_mode_it = std::find(present_modes.begin(),
                                               present_modes.end(),
                                               vk::PresentModeKHR::eMailbox);
        if (present_mode_it != present_modes.end())
        {
            present_mode = *present_mode_it;
        }
    }

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

    auto image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 &&
        image_count > surface_capabilities.maxImageCount)
    {
        image_count = surface_capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_create_info {
        .surface = surface,
        .minImageCount = image_count,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = surface_capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = present_mode,
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

    return Swapchain {.swapchain =
                          vk::raii::SwapchainKHR(device, swapchain_create_info),
                      .format = surface_format.format,
                      .extent = extent};
}

std::vector<vk::Image>
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

std::vector<vk::raii::ImageView>
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

vk::raii::ImageView create_image_view(const vk::raii::Device &device,
                                      vk::Image image,
                                      vk::Format format)
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

vk::raii::CommandPool create_command_pool(const vk::raii::Device &device,
                                          std::uint32_t graphics_family_index)
{
    const vk::CommandPoolCreateInfo create_info {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphics_family_index};

    return {device, create_info};
}

vk::raii::CommandBuffer
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

std::uint32_t find_memory_type(const vk::raii::PhysicalDevice &physical_device,
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

Buffer create_buffer(const vk::raii::Device &device,
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

Image create_image(const vk::raii::Device &device,
                   const vk::raii::PhysicalDevice &physical_device,
                   std::uint32_t width,
                   std::uint32_t height,
                   vk::Format format,
                   vk::ImageTiling tiling,
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
        .tiling = tiling,
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

void command_blit_image(const vk::raii::CommandBuffer &command_buffer,
                        vk::Image src_image,
                        std::int32_t src_width,
                        std::int32_t src_height,
                        vk::Image dst_image,
                        std::int32_t dst_width,
                        std::int32_t dst_height)
{
    const vk::ImageBlit region {
        .srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .mipLevel = 0,
                           .baseArrayLayer = 0,
                           .layerCount = 1},
        .srcOffsets = {{vk::Offset3D {0, 0, 0},
                        vk::Offset3D {src_width, src_height, 1}}},
        .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .mipLevel = 0,
                           .baseArrayLayer = 0,
                           .layerCount = 1},
        .dstOffsets = {
            {vk::Offset3D {0, 0, 0}, vk::Offset3D {dst_width, dst_height, 1}}}};

    command_buffer.blitImage(src_image,
                             vk::ImageLayout::eTransferSrcOptimal,
                             dst_image,
                             vk::ImageLayout::eTransferDstOptimal,
                             region,
                             vk::Filter::eNearest);
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

vk::raii::RenderPass create_render_pass(const vk::raii::Device &device,
                                        vk::Format swapchain_format)
{
    const vk::AttachmentDescription color_attachment_description {
        .format = swapchain_format,
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

vk::raii::DescriptorSetLayout
create_descriptor_set_layout(const vk::raii::Device &device)
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

vk::raii::PipelineLayout create_pipeline_layout(
    const vk::raii::Device &device,
    const vk::raii::DescriptorSetLayout &descriptor_set_layout)
{
    const vk::PipelineLayoutCreateInfo create_info {
        .setLayoutCount = 1, .pSetLayouts = &*descriptor_set_layout};

    return {device, create_info};
}

vk::raii::Pipeline create_pipeline(
    const vk::raii::Device &device,
    const char *vertex_shader_path,
    const char *fragment_shader_path,
    const vk::Extent2D &swapchain_extent,
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
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(swapchain_extent.width),
        .height = static_cast<float>(swapchain_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f};

    const vk::Rect2D scissor {.offset = {0, 0}, .extent = swapchain_extent};

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

vk::raii::ShaderModule
create_shader_module(const vk::raii::Device &device,
                     const std::vector<std::uint8_t> &shader_code)
{
    const vk::ShaderModuleCreateInfo create_info {
        .codeSize = shader_code.size(),
        .pCode = reinterpret_cast<const std::uint32_t *>(shader_code.data())};

    return {device, create_info};
}

std::vector<vk::raii::Framebuffer> create_framebuffers(
    const vk::raii::Device &device,
    const std::vector<vk::raii::ImageView> &swapchain_image_views,
    vk::RenderPass render_pass,
    const vk::Extent2D &swapchain_extent)
{
    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(swapchain_image_views.size());

    for (const auto &image_view : swapchain_image_views)
    {
        const vk::FramebufferCreateInfo framebuffer_info {
            .renderPass = render_pass,
            .attachmentCount = 1,
            .pAttachments = &*image_view,
            .width = swapchain_extent.width,
            .height = swapchain_extent.height,
            .layers = 1};
        framebuffers.emplace_back(device, framebuffer_info);
    }

    return framebuffers;
}

vk::raii::Sampler create_sampler(const vk::raii::Device &device)
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

Image create_texture_image(const vk::raii::Device &device,
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
                              vk::ImageTiling::eOptimal,
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

    command_copy_image_to_buffer(command_buffer,
                                 image,
                                 *staging_buffer.buffer,
                                 static_cast<std::uint32_t>(width),
                                 static_cast<std::uint32_t>(height));

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

vk::raii::DescriptorPool create_descriptor_pool(const vk::raii::Device &device)
{
    constexpr std::array pool_sizes {
        vk::DescriptorPoolSize {.type = vk::DescriptorType::eUniformBuffer,
                                .descriptorCount = max_frames_in_flight},
        vk::DescriptorPoolSize {.type =
                                    vk::DescriptorType::eCombinedImageSampler,
                                .descriptorCount = max_frames_in_flight}};

    const vk::DescriptorPoolCreateInfo create_info {
        .maxSets = max_frames_in_flight,
        .poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data()};

    return {device, create_info};
}

std::vector<vk::DescriptorSet>
create_descriptor_sets(const vk::raii::Device &device,
                       vk::DescriptorSetLayout descriptor_set_layout,
                       vk::DescriptorPool descriptor_pool,
                       vk::Sampler sampler,
                       vk::ImageView texture_image_view,
                       const std::vector<Buffer> &uniform_buffers,
                       vk::DeviceSize uniform_buffer_size)
{
    std::array<vk::DescriptorSetLayout, max_frames_in_flight> layouts;
    std::fill(layouts.begin(), layouts.end(), descriptor_set_layout);

    const vk::DescriptorSetAllocateInfo allocate_info {
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = max_frames_in_flight,
        .pSetLayouts = layouts.data()};

    std::vector<vk::DescriptorSet> descriptor_sets {
        (*device).allocateDescriptorSets(allocate_info)};

    for (std::size_t i {}; i < max_frames_in_flight; ++i)
    {
        const vk::DescriptorBufferInfo buffer_info {
            .buffer = *uniform_buffers[i].buffer,
            .offset = 0,
            .range = uniform_buffer_size};

        const vk::DescriptorImageInfo image_info {
            .sampler = sampler,
            .imageView = texture_image_view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

        const std::array descriptor_writes {
            vk::WriteDescriptorSet {.dstSet = descriptor_sets[i],
                                    .dstBinding = 0,
                                    .dstArrayElement = 0,
                                    .descriptorCount = 1,
                                    .descriptorType =
                                        vk::DescriptorType::eUniformBuffer,
                                    .pBufferInfo = &buffer_info},
            vk::WriteDescriptorSet {
                .dstSet = descriptor_sets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &image_info}};

        device.updateDescriptorSets(descriptor_writes, {});
    }

    return descriptor_sets;
}

Buffer create_vertex_buffer(const vk::raii::Device &device,
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

    auto *const data = staging_buffer.memory.mapMemory(0, vertex_buffer_size);
    std::memcpy(
        data, vertex_data, static_cast<std::size_t>(vertex_buffer_size));
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

Buffer create_index_buffer(const vk::raii::Device &device,
                           const vk::raii::PhysicalDevice &physical_device,
                           const vk::raii::CommandPool &command_pool,
                           const vk::raii::Queue &graphics_queue,
                           const std::vector<std::uint16_t> &indices)
{
    const vk::DeviceSize buffer_size {sizeof(std::uint16_t) * indices.size()};

    const auto staging_buffer =
        create_buffer(device,
                      physical_device,
                      buffer_size,
                      vk::BufferUsageFlagBits::eTransferSrc,
                      vk::MemoryPropertyFlagBits::eHostVisible |
                          vk::MemoryPropertyFlagBits::eHostCoherent);

    auto *const data = staging_buffer.memory.mapMemory(0, buffer_size);
    std::memcpy(data, indices.data(), static_cast<std::size_t>(buffer_size));
    staging_buffer.memory.unmapMemory();

    auto index_buffer = create_buffer(device,
                                      physical_device,
                                      buffer_size,
                                      vk::BufferUsageFlagBits::eTransferDst |
                                          vk::BufferUsageFlagBits::eIndexBuffer,
                                      vk::MemoryPropertyFlagBits::eDeviceLocal);

    const auto command_buffer =
        begin_one_time_submit_command_buffer(device, command_pool);

    command_copy_buffer(command_buffer,
                        *staging_buffer.buffer,
                        *index_buffer.buffer,
                        buffer_size);

    end_one_time_submit_command_buffer(command_buffer, graphics_queue);

    return index_buffer;
}

std::vector<Buffer>
create_uniform_buffers(const vk::raii::Device &device,
                       const vk::raii::PhysicalDevice &physical_device,
                       vk::DeviceSize uniform_buffer_size)
{
    std::vector<Buffer> uniform_buffers;
    uniform_buffers.reserve(max_frames_in_flight);

    for (std::size_t i {}; i < max_frames_in_flight; ++i)
    {
        uniform_buffers.push_back(
            create_buffer(device,
                          physical_device,
                          uniform_buffer_size,
                          vk::BufferUsageFlagBits::eUniformBuffer,
                          vk::MemoryPropertyFlagBits::eHostVisible |
                              vk::MemoryPropertyFlagBits::eHostCoherent));
    }

    return uniform_buffers;
}

vk::raii::CommandBuffers
create_frame_command_buffers(const vk::raii::Device &device,
                             const vk::raii::CommandPool &command_pool)
{
    const vk::CommandBufferAllocateInfo allocate_info {
        .commandPool = *command_pool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = max_frames_in_flight};

    return {device, allocate_info};
}

void record_frame_command_buffer(const vk::raii::CommandBuffer &command_buffer,
                                 vk::RenderPass render_pass,
                                 vk::Framebuffer framebuffer,
                                 vk::Extent2D swapchain_extent,
                                 vk::Pipeline pipeline,
                                 vk::PipelineLayout pipeline_layout,
                                 vk::DescriptorSet descriptor_set,
                                 vk::Buffer vertex_buffer,
                                 vk::Buffer index_buffer,
                                 std::uint32_t num_indices)
{
    command_buffer.begin({});

    constexpr vk::ClearValue clear_color_value {
        .color = {{{0.0f, 0.0f, 0.0f, 1.0f}}}};

    const vk::RenderPassBeginInfo render_pass_begin_info {
        .renderPass = render_pass,
        .framebuffer = framebuffer,
        .renderArea = {.offset = {0, 0}, .extent = swapchain_extent},
        .clearValueCount = 1,
        .pClearValues = &clear_color_value};

    command_buffer.beginRenderPass(render_pass_begin_info,
                                   vk::SubpassContents::eInline);

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

    command_buffer.bindVertexBuffers(0, vertex_buffer, {0});

    command_buffer.bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint16);

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                      pipeline_layout,
                                      0,
                                      descriptor_set,
                                      {});

    command_buffer.drawIndexed(num_indices, 1, 0, 0, 0);

    command_buffer.endRenderPass();

    command_buffer.end();
}
