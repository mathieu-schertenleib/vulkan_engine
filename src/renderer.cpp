#include "renderer.hpp"

#include "utils.hpp"

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
    std::cerr << callback_data->pMessage << std::endl;
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

[[nodiscard]] vk::raii::Instance create_instance()
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

constexpr vk::VertexInputBindingDescription vertex_binding_description {
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = vk::VertexInputRate::eVertex};

constexpr std::array vertex_attribute_descriptions {
    vk::VertexInputAttributeDescription {.location = 0,
                                         .binding = 0,
                                         .format = vk::Format::eR32G32B32Sfloat,
                                         .offset = offsetof(Vertex, pos)},
    vk::VertexInputAttributeDescription {.location = 1,
                                         .binding = 0,
                                         .format = vk::Format::eR32G32Sfloat,
                                         .offset =
                                             offsetof(Vertex, tex_coord)}};

} // namespace

Renderer::Renderer(GLFWwindow *window,
                   std::uint32_t width,
                   std::uint32_t height)
    : m_instance {create_instance()}
#ifdef ENABLE_VALIDATION_LAYERS
    , m_debug_messenger {m_instance, debug_utils_messenger_create_info}
#endif
    , m_surface {create_surface(window)}
    , m_physical_device {select_physical_device()}
    , m_queue_family_indices {get_queue_family_indices(m_physical_device)
                                  .value()}
    , m_device {create_device()}
    , m_graphics_queue {get_graphics_queue()}
    , m_present_queue {get_present_queue()}
    , m_swapchain {create_swapchain(width, height)}
    , m_swapchain_images {get_swapchain_images()}
    , m_swapchain_image_views {create_swapchain_image_views()}
    , m_render_pass {create_render_pass()}
    , m_descriptor_set_layout {create_descriptor_set_layout()}
    , m_pipeline_layout {create_pipeline_layout()}
    , m_pipeline {create_pipeline()}
    , m_framebuffers {create_framebuffers()}
{
}

Renderer::~Renderer()
{
    m_device.waitIdle();
}

vk::raii::SurfaceKHR Renderer::create_surface(GLFWwindow *window)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(*m_instance, window, nullptr, &surface) !=
        VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create surface");
    }
    return {m_instance, surface};
}

vk::raii::PhysicalDevice Renderer::select_physical_device()
{
    vk::raii::PhysicalDevices physical_devices(m_instance);

    if (physical_devices.empty())
    {
        throw std::runtime_error("Failed to find a device with Vulkan support");
    }

    std::vector<vk::raii::PhysicalDevice> suitable_devices;
    for (const auto &physical_device : physical_devices)
    {
        if (is_physical_device_suitable(physical_device))
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

bool Renderer::is_physical_device_suitable(
    const vk::raii::PhysicalDevice &physical_device)
{
    if (!swapchain_extension_supported(physical_device))
    {
        return false;
    }

    if (physical_device.getSurfaceFormatsKHR(*m_surface).empty())
    {
        return false;
    }

    if (physical_device.getSurfacePresentModesKHR(*m_surface).empty())
    {
        return false;
    }

    if (!get_queue_family_indices(physical_device).has_value())
    {
        return false;
    }

    return true;
}

std::optional<Queue_family_indices> Renderer::get_queue_family_indices(
    const vk::raii::PhysicalDevice &physical_device)
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

        if (physical_device.getSurfaceSupportKHR(i, *m_surface))
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

vk::raii::Device Renderer::create_device()
{
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

    std::set<std::uint32_t> unique_queue_family_indices {
        m_queue_family_indices.graphics, m_queue_family_indices.present};

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

    return {m_physical_device, device_create_info};
}

vk::raii::Queue Renderer::get_graphics_queue()
{
    return m_device.getQueue(m_queue_family_indices.graphics, 0);
}

vk::raii::Queue Renderer::get_present_queue()
{
    return m_device.getQueue(m_queue_family_indices.present, 0);
}

Swapchain Renderer::create_swapchain(std::uint32_t width, std::uint32_t height)
{
    const auto surface_formats =
        m_physical_device.getSurfaceFormatsKHR(*m_surface);
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

    const auto present_modes =
        m_physical_device.getSurfacePresentModesKHR(*m_surface);
    const auto present_mode_it = std::find(present_modes.begin(),
                                           present_modes.end(),
                                           vk::PresentModeKHR::eMailbox);
    const auto present_mode = (present_mode_it != present_modes.end())
                                  ? *present_mode_it
                                  : vk::PresentModeKHR::eFifo;

    const auto surface_capabilities =
        m_physical_device.getSurfaceCapabilitiesKHR(*m_surface);
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
        .surface = *m_surface,
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

    const std::uint32_t queue_family_indices[] {m_queue_family_indices.graphics,
                                                m_queue_family_indices.present};
    if (m_queue_family_indices.graphics != m_queue_family_indices.present)
    {
        swapchain_create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchain_create_info.queueFamilyIndexCount = 2;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        swapchain_create_info.imageSharingMode = vk::SharingMode::eExclusive;
        swapchain_create_info.queueFamilyIndexCount = 1;
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
    }
    return Swapchain {
        .swapchain = vk::raii::SwapchainKHR(m_device, swapchain_create_info),
        .format = surface_format.format,
        .extent = extent};
}

std::vector<vk::Image> Renderer::get_swapchain_images() const
{
    const auto images = m_swapchain.swapchain.getImages();
    std::vector<vk::Image> result;
    result.reserve(images.size());
    for (const auto &image : images)
    {
        result.emplace_back(image);
    }
    return result;
}

std::vector<vk::raii::ImageView> Renderer::create_swapchain_image_views()
{
    std::vector<vk::raii::ImageView> swapchain_image_views;
    swapchain_image_views.reserve(m_swapchain_images.size());

    for (const auto &swapchain_image : m_swapchain_images)
    {
        swapchain_image_views.push_back(
            create_image_view(swapchain_image, m_swapchain.format));
    }

    return swapchain_image_views;
}

vk::raii::ImageView Renderer::create_image_view(const vk::Image &image,
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

    return {m_device, create_info};
}

vk::raii::RenderPass Renderer::create_render_pass()
{
    const vk::AttachmentDescription color_attachment_description {
        .format = m_swapchain.format,
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

    const vk::SubpassDependency subpass_dependency {
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

    return {m_device, create_info};
}

vk::raii::DescriptorSetLayout Renderer::create_descriptor_set_layout()
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

    return {m_device, create_info};
}

vk::raii::PipelineLayout Renderer::create_pipeline_layout()
{
    const vk::PipelineLayoutCreateInfo create_info {
        .setLayoutCount = 1, .pSetLayouts = &*m_descriptor_set_layout};

    return {m_device, create_info};
}

vk::raii::Pipeline Renderer::create_pipeline()
{
    constexpr auto vertex_shader_path = "shaders/spv/shader.vert.spv";
    const auto vertex_shader_code = load_binary_file(vertex_shader_path);
    if (vertex_shader_code.empty())
    {
        throw std::runtime_error(std::string("Failed to load shader \"") +
                                 std::string(vertex_shader_path) +
                                 std::string("\""));
    }
    constexpr auto fragment_shader_path = "shaders/spv/shader.frag.spv";
    const auto fragment_shader_code = load_binary_file(fragment_shader_path);
    if (fragment_shader_code.empty())
    {
        throw std::runtime_error(std::string("Failed to load shader \"") +
                                 std::string(fragment_shader_path) +
                                 std::string("\""));
    }

    const auto vertex_shader_module = create_shader_module(vertex_shader_code);
    const auto fragment_shader_module =
        create_shader_module(fragment_shader_code);

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

    constexpr vk::PipelineVertexInputStateCreateInfo
        vertex_input_state_create_info {
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertex_binding_description,
            .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(
                vertex_attribute_descriptions.size()),
            .pVertexAttributeDescriptions =
                vertex_attribute_descriptions.data()};

    const vk::PipelineInputAssemblyStateCreateInfo
        input_assembly_state_create_info {
            .topology = vk::PrimitiveTopology::eTriangleStrip,
            .primitiveRestartEnable = VK_FALSE};

    const vk::Viewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(m_swapchain.extent.width),
        .height = static_cast<float>(m_swapchain.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f};

    const vk::Rect2D scissor {.offset = {0, 0}, .extent = m_swapchain.extent};

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
        .layout = *m_pipeline_layout,
        .renderPass = *m_render_pass,
        .subpass = 0};

    return {m_device, VK_NULL_HANDLE, pipeline_create_info};
}

vk::raii::ShaderModule
Renderer::create_shader_module(const std::vector<std::uint8_t> &shader_code)
{
    const vk::ShaderModuleCreateInfo create_info {
        .codeSize = shader_code.size(),
        .pCode = reinterpret_cast<const std::uint32_t *>(shader_code.data())};

    return {m_device, create_info};
}

std::vector<vk::raii::Framebuffer> Renderer::create_framebuffers()
{
    std::vector<vk::raii::Framebuffer> framebuffers;
    framebuffers.reserve(m_swapchain_image_views.size());
    for (const auto &image_view : m_swapchain_image_views)
    {
        const vk::FramebufferCreateInfo framebufferInfo {
            .renderPass = *m_render_pass,
            .attachmentCount = 1,
            .pAttachments = &*image_view,
            .width = m_swapchain.extent.width,
            .height = m_swapchain.extent.height,
            .layers = 1};
        framebuffers.emplace_back(m_device, framebufferInfo);
    }
    return framebuffers;
}