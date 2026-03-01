#include "vulkan_context.h"

#define VOLK_IMPLEMENTATION
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>
#include "SDL3/SDL_vulkan.h"
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1004000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

#include "qhenki/utility/string_util.h"

using namespace qhenki::gfx;

namespace
{
#define DXGI_VK_FORMAT_MAP(X)                                            \
    X(DXGI_FORMAT_R32G32B32A32_FLOAT, VK_FORMAT_R32G32B32A32_SFLOAT)     \
    X(DXGI_FORMAT_R32G32B32A32_UINT, VK_FORMAT_R32G32B32A32_UINT)        \
    X(DXGI_FORMAT_R32G32B32A32_SINT, VK_FORMAT_R32G32B32A32_SINT)        \
    X(DXGI_FORMAT_R32G32B32_FLOAT, VK_FORMAT_R32G32B32_SFLOAT)           \
    X(DXGI_FORMAT_R32G32B32_UINT, VK_FORMAT_R32G32B32_UINT)              \
    X(DXGI_FORMAT_R32G32B32_SINT, VK_FORMAT_R32G32B32_SINT)              \
    X(DXGI_FORMAT_R16G16B16A16_FLOAT, VK_FORMAT_R16G16B16A16_SFLOAT)     \
    X(DXGI_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_UNORM)      \
    X(DXGI_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT)        \
    X(DXGI_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_SNORM)      \
    X(DXGI_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT)        \
    X(DXGI_FORMAT_R32G32_FLOAT, VK_FORMAT_R32G32_SFLOAT)                 \
    X(DXGI_FORMAT_R32G32_UINT, VK_FORMAT_R32G32_UINT)                    \
    X(DXGI_FORMAT_R32G32_SINT, VK_FORMAT_R32G32_SINT)                    \
    X(DXGI_FORMAT_R10G10B10A2_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32) \
    X(DXGI_FORMAT_R10G10B10A2_UINT, VK_FORMAT_A2B10G10R10_UINT_PACK32)   \
    X(DXGI_FORMAT_R11G11B10_FLOAT, VK_FORMAT_B10G11R11_UFLOAT_PACK32)    \
    X(DXGI_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM)              \
    X(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, VK_FORMAT_R8G8B8A8_SRGB)          \
    X(DXGI_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT)                \
    X(DXGI_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM)              \
    X(DXGI_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT)                \
    X(DXGI_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)              \
    X(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, VK_FORMAT_B8G8R8A8_SRGB)          \
    X(DXGI_FORMAT_B8G8R8X8_UNORM, VK_FORMAT_B8G8R8A8_UNORM)              \
    X(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, VK_FORMAT_B8G8R8A8_SRGB)          \
    X(DXGI_FORMAT_R16G16_FLOAT, VK_FORMAT_R16G16_SFLOAT)                 \
    X(DXGI_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16_UNORM)                  \
    X(DXGI_FORMAT_R16G16_UINT, VK_FORMAT_R16G16_UINT)                    \
    X(DXGI_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16_SNORM)                  \
    X(DXGI_FORMAT_R16G16_SINT, VK_FORMAT_R16G16_SINT)                    \
    X(DXGI_FORMAT_R32_FLOAT, VK_FORMAT_R32_SFLOAT)                       \
    X(DXGI_FORMAT_R32_UINT, VK_FORMAT_R32_UINT)                          \
    X(DXGI_FORMAT_R32_SINT, VK_FORMAT_R32_SINT)                          \
    X(DXGI_FORMAT_D32_FLOAT, VK_FORMAT_D32_SFLOAT)                       \
    X(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, VK_FORMAT_D32_SFLOAT_S8_UINT)    \
    X(DXGI_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT)        \
    X(DXGI_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM)                        \
    X(DXGI_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8_UNORM)                      \
    X(DXGI_FORMAT_R8G8_UINT, VK_FORMAT_R8G8_UINT)                        \
    X(DXGI_FORMAT_R8G8_SNORM, VK_FORMAT_R8G8_SNORM)                      \
    X(DXGI_FORMAT_R8G8_SINT, VK_FORMAT_R8G8_SINT)                        \
    X(DXGI_FORMAT_R16_FLOAT, VK_FORMAT_R16_SFLOAT)                       \
    X(DXGI_FORMAT_R16_UNORM, VK_FORMAT_R16_UNORM)                        \
    X(DXGI_FORMAT_R16_UINT, VK_FORMAT_R16_UINT)                          \
    X(DXGI_FORMAT_R16_SNORM, VK_FORMAT_R16_SNORM)                        \
    X(DXGI_FORMAT_R16_SINT, VK_FORMAT_R16_SINT)                          \
    X(DXGI_FORMAT_R8_UNORM, VK_FORMAT_R8_UNORM)                          \
    X(DXGI_FORMAT_R8_UINT, VK_FORMAT_R8_UINT)                            \
    X(DXGI_FORMAT_R8_SNORM, VK_FORMAT_R8_SNORM)                          \
    X(DXGI_FORMAT_R8_SINT, VK_FORMAT_R8_SINT)                            \
    X(DXGI_FORMAT_A8_UNORM, VK_FORMAT_R8_UNORM)                          \
    X(DXGI_FORMAT_BC1_UNORM, VK_FORMAT_BC1_RGBA_UNORM_BLOCK)             \
    X(DXGI_FORMAT_BC1_UNORM_SRGB, VK_FORMAT_BC1_RGBA_SRGB_BLOCK)         \
    X(DXGI_FORMAT_BC2_UNORM, VK_FORMAT_BC2_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC2_UNORM_SRGB, VK_FORMAT_BC2_SRGB_BLOCK)              \
    X(DXGI_FORMAT_BC3_UNORM, VK_FORMAT_BC3_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC3_UNORM_SRGB, VK_FORMAT_BC3_SRGB_BLOCK)              \
    X(DXGI_FORMAT_BC4_UNORM, VK_FORMAT_BC4_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC4_SNORM, VK_FORMAT_BC4_SNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC5_UNORM, VK_FORMAT_BC5_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC5_SNORM, VK_FORMAT_BC5_SNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC6H_UF16, VK_FORMAT_BC6H_UFLOAT_BLOCK)                \
    X(DXGI_FORMAT_BC6H_SF16, VK_FORMAT_BC6H_SFLOAT_BLOCK)                \
    X(DXGI_FORMAT_BC7_UNORM, VK_FORMAT_BC7_UNORM_BLOCK)                  \
    X(DXGI_FORMAT_BC7_UNORM_SRGB, VK_FORMAT_BC7_SRGB_BLOCK)              \
    X(DXGI_FORMAT_B5G6R5_UNORM, VK_FORMAT_R5G6B5_UNORM_PACK16)           \
    X(DXGI_FORMAT_B5G5R5A1_UNORM, VK_FORMAT_A1R5G5B5_UNORM_PACK16)       \
    X(DXGI_FORMAT_B4G4R4A4_UNORM, VK_FORMAT_B4G4R4A4_UNORM_PACK16)       \
    X(DXGI_FORMAT_R9G9B9E5_SHAREDEXP, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)

VkFormat convert_format(const DXGI_FORMAT format)
{
#define MAP_DXGI_TO_VK(dxgi, vk) \
    case dxgi:                   \
        return vk;

    switch (format)
    {
        DXGI_VK_FORMAT_MAP(MAP_DXGI_TO_VK)
    default:
        return VK_FORMAT_UNDEFINED;
    }

#undef MAP_DXGI_TO_VK
}

#undef DXGI_VK_FORMAT_MAP
} // namespace

constexpr uint32_t major = 1;
constexpr uint32_t minor = 4;

std::string VulkanContext::create(const bool enable_debug_layer)
{
    if (volkInitialize() != VK_SUCCESS)
    {
        return "Vulkan: Failed to initialize Volk";
    }

    vkb::InstanceBuilder builder;

    builder.set_engine_name("QhenkiX").set_engine_version(0, 1, 0);

    std::array instance_extensions = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
    };
    builder.enable_extensions(instance_extensions);

    if (enable_debug_layer)
    {
        constexpr auto severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
        builder.request_validation_layers().set_debug_messenger_severity(severity).set_debug_callback(
            [](const VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               const VkDebugUtilsMessageTypeFlagsEXT message_type,
               const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
               void* p_user_data) -> VkBool32
            {
                const auto sev = message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT   ? "ERROR"
                               : message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? "WARNING"
                               : message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    ? "INFO"
                                                                                                    : "VERBOSE";
                const auto type = message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     ? "GENERAL"
                                : message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  ? "VALIDATION"
                                : message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT ? "PERFORMANCE"
                                                                                                 : "UNKNOWN";
                const auto id_name = p_callback_data->pMessageIdName ? p_callback_data->pMessageIdName : "UNKNOWN";

#define MESSAGE                                               \
    "Vulkan %s: %s [VK %s #%d: %s]\n", sev,                   \
        p_callback_data->pMessage ? p_callback_data->pMessage \
                                                              \
                                  : "(null)",                 \
        type, static_cast<int>(p_callback_data->messageIdNumber), id_name

#if defined(_WIN32) || defined(_WIN64)
                const auto msg = qhenki::util::format_string<1024>(MESSAGE);
                OutputDebugStringA(msg.buffer.data());
#else
                printf(MESSAGE);
#endif
                return VK_FALSE;
#undef MESSAGE
            });
    }

    auto inst_ret = builder.require_api_version(major, minor).build();
    if (!inst_ret)
    {
        return "Vulkan: Failed to create instance";
    }
    m_instance = inst_ret.value();

    volkLoadInstanceOnly(m_instance.instance);

    // Since this is Vulkan 1.4 most of the stuff we need is core
    std::array<const char*, 4> device_extensions{
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_9_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
    };

    vkb::PhysicalDeviceSelector selector{m_instance};
    static_assert(minor >= 4);

    VkPhysicalDeviceFeatures features{
        .robustBufferAccess = VK_TRUE,
        .fullDrawIndexUint32 = VK_TRUE,
        .imageCubeArray = VK_TRUE,
        .independentBlend = VK_TRUE,
        .logicOp = VK_TRUE,
        .multiDrawIndirect = VK_TRUE,
        .drawIndirectFirstInstance = VK_TRUE,
        .multiViewport = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        //.vertexPipelineStoresAndAtomics = VK_TRUE,
        //.fragmentStoresAndAtomics = VK_TRUE,
        .shaderStorageImageExtendedFormats = VK_TRUE,
        .shaderUniformBufferArrayDynamicIndexing = VK_TRUE,
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
        .shaderStorageBufferArrayDynamicIndexing = VK_TRUE,
        .shaderStorageImageArrayDynamicIndexing = VK_TRUE,
    };

    VkPhysicalDeviceVulkan11Features features11{
        .multiview = VK_TRUE,
    };

    VkPhysicalDeviceVulkan12Features features12{
        .drawIndirectCount = VK_TRUE,
        .descriptorIndexing = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features features13{
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
        .maintenance4 = VK_TRUE,
    };

    VkPhysicalDeviceVulkan14Features features14{
        .maintenance5 = VK_TRUE,
        .maintenance6 = VK_TRUE,
        .pushDescriptor = VK_TRUE,
    };

    auto phys_ret = selector.defer_surface_initialization()
                        .set_minimum_version(major, minor)
                        .add_required_extensions(device_extensions)
                        .set_required_features(features)
                        .set_required_features_11(features11)
                        .set_required_features_12(features12)
                        .set_required_features_13(features13)
                        .set_required_features_14(features14)
                        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                        .select();

    if (!phys_ret)
    {
        return "Vulkan: Failed to select physical device" + phys_ret.error().message();
    }

    vkb::DeviceBuilder device_builder{phys_ret.value()};
    auto dev_ret = device_builder.build();
    if (!dev_ret)
    {
        return "Vulkan: Failed to create logical device" + dev_ret.error().message();
    }
    m_device = dev_ret.value();

    volkLoadDevice(m_device.device);

    VmaVulkanFunctions vulkan_functions{};
    VmaAllocatorCreateInfo allocator_desc{
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT |
                 VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT,
        .physicalDevice = m_device.physical_device,
        .device = m_device.device,
        .instance = m_instance.instance,
        .vulkanApiVersion = VK_API_VERSION_1_4,
    };

    if (vmaImportVulkanFunctionsFromVolk(&allocator_desc, &vulkan_functions) != VK_SUCCESS)
    {
        return "Vulkan: Failed to import Vulkan functions for VMA from Volk";
    }

    allocator_desc.pVulkanFunctions = &vulkan_functions;

    if (vmaCreateAllocator(&allocator_desc, &m_allocator) != VK_SUCCESS)
    {
        return "Vulkan: Failed to create VMA allocator";
    }

    VkPhysicalDeviceProperties2 device_props2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    device_props2.pNext = &m_capabilities.descriptor_heap_properties;
    vkGetPhysicalDeviceProperties2(m_device.physical_device, &device_props2);

    return "";
}

bool VulkanContext::is_compatibility() const
{
    return false;
}

bool VulkanContext::create_swapchain(const DisplayWindow& window,
                                     const SwapchainDesc& swapchain_desc,
                                     Queue* direct_queue,
                                     unsigned* frame_index)
{
    VkSurfaceKHR surface;
    const auto status = SDL_Vulkan_CreateSurface(window.get_window(), m_instance, nullptr, &surface);
    if (!status)
    {
        return false;
    }

    vkb::SwapchainBuilder swapchain_builder{m_device, surface};

    const VkPresentModeKHR present_mode = swapchain_desc.tearing ? VK_PRESENT_MODE_IMMEDIATE_KHR
                                                                 : VK_PRESENT_MODE_FIFO_KHR;
    auto swap_ret = swapchain_builder.set_desired_extent(swapchain_desc.width, swapchain_desc.height)
                        .set_desired_present_mode(present_mode)
                        .set_required_min_image_count(swapchain_desc.buffer_count)
                        .set_desired_format({convert_format(swapchain_desc.format), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                        .build();
    if (!swap_ret)
    {
        return false;
    }
    m_swapchain = swap_ret.value();
    return true;
}

bool VulkanContext::resize_swapchain(Swapchain* swapchain, int width, int height, unsigned& frame_index)
{
    return true;
}

bool VulkanContext::present(const Swapchain& swapchain,
                            unsigned fence_count,
                            Fence* wait_fences,
                            unsigned swapchain_index)
{
    return true;
}

unsigned VulkanContext::get_swapchain_frame_index(const Swapchain& swapchain)
{
    return 0;
}

bool VulkanContext::create_shader(void* data, size_t size, ShaderType type, Shader* shader)
{
    return true;
}

bool VulkanContext::create_pipeline(const GraphicsPipelineDesc& desc,
                                    GraphicsPipeline* pipeline,
                                    const Shader& vertex_shader,
                                    const Shader& pixel_shader,
                                    PipelineLayout* in_layout,
                                    const char* debug_name)
{
    return true;
}

bool VulkanContext::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
    return true;
}

bool VulkanContext::create_pipeline_layout(PipelineLayoutDesc* desc, PipelineLayout* layout)
{
    return true;
}

void VulkanContext::bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout)
{
}

bool VulkanContext::set_pipeline_constant(
    CommandList* cmd_list, unsigned param, uint32_t offset, unsigned size, void* data)
{
    return true;
}

bool VulkanContext::create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap* heap, const char* debug_name)
{
    heap->desc = desc;

    return true;
}

size_t VulkanContext::get_descriptor_heap_max_size(const DescriptorHeapDesc::Type type) const
{
    if (type == DescriptorHeapDesc::Type::SAMPLER)
    {
        return m_capabilities.descriptor_heap_properties.maxSamplerHeapSize;
    }
    return m_capabilities.descriptor_heap_properties.maxResourceHeapSize;
}

void VulkanContext::set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap)
{
}

void VulkanContext::set_descriptor_heap(CommandList* cmd_list,
                                        const DescriptorHeap& heap,
                                        const DescriptorHeap& sampler_heap)
{
}

void VulkanContext::set_descriptor_table(CommandList* cmd_list, unsigned index, const Descriptor& gpu_descriptor)
{
}

bool VulkanContext::copy_descriptors(size_t bytes, const Descriptor& src, const Descriptor& dst)
{
    return true;
}

bool VulkanContext::free_descriptor(Descriptor* descriptor)
{
    return true;
}

size_t VulkanContext::get_descriptor_size(const Descriptor::Type type) const
{
    switch (type)
    {
    case Descriptor::BUFFER:
        return m_capabilities.descriptor_heap_properties.bufferDescriptorSize;
    case Descriptor::TEXTURE:
        return m_capabilities.descriptor_heap_properties.imageDescriptorSize;
    case Descriptor::SAMPLER:
        return m_capabilities.descriptor_heap_properties.samplerDescriptorSize;
    }
    return 0;
}

size_t VulkanContext::get_descriptor_alignment(const Descriptor::Type type) const
{
    switch (type)
    {
    case Descriptor::BUFFER:
        return m_capabilities.descriptor_heap_properties.bufferDescriptorAlignment;
    case Descriptor::TEXTURE:
        return m_capabilities.descriptor_heap_properties.imageDescriptorAlignment;
    case Descriptor::SAMPLER:
        return m_capabilities.descriptor_heap_properties.samplerDescriptorAlignment;
    }
    return 0;
}

bool VulkanContext::create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, const char* debug_name)
{
    return true;
}

bool VulkanContext::create_descriptor_constant_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor)
{
    return true;
}

bool VulkanContext::create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor)
{
    return true;
}

void VulkanContext::copy_buffer(
    CommandList* cmd_list, const Buffer& src, uint64_t src_offset, Buffer* dst, uint64_t dst_offset, uint64_t bytes)
{
}

bool VulkanContext::create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name)
{
    return true;
}

bool VulkanContext::create_descriptor_shader_view(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor)
{
    return false;
}

bool VulkanContext::copy_to_texture(CommandList* cmd_list, const void* data, Buffer* staging, Texture* texture)
{
    return true;
}

bool VulkanContext::create_descriptor(const SamplerDesc& desc, DescriptorHeap* heap, Descriptor* descriptor)
{
    return true;
}

void* VulkanContext::map_buffer(const Buffer& buffer)
{
    return nullptr;
}

void VulkanContext::unmap_buffer(const Buffer& buffer)
{
}

void VulkanContext::bind_vertex_buffers(CommandList* cmd_list,
                                        unsigned start_slot,
                                        unsigned buffer_count,
                                        const Buffer* const* buffers,
                                        const unsigned* sizes,
                                        const unsigned* strides,
                                        const unsigned* offsets)
{
}

void VulkanContext::bind_index_buffer(CommandList* cmd_list, const Buffer& buffer, IndexType format, unsigned offset)
{
}

bool VulkanContext::create_queue(QueueType type, Queue* queue)
{
    return true;
}

bool VulkanContext::create_command_pool(CommandPool* command_pool, const Queue& queue)
{
    return true;
}

bool VulkanContext::create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name)
{
    return true;
}

bool VulkanContext::reset_command_list(CommandList* cmd_list, const CommandPool& command_pool)
{
    return true;
}

bool VulkanContext::close_command_list(CommandList* cmd_list)
{
    return true;
}

bool VulkanContext::reset_command_pool(CommandPool* command_pool)
{
    return true;
}

bool VulkanContext::start_render_pass(CommandList* cmd_list,
                                      const float* clear_color_values,
                                      const RenderTarget* depth_stencil,
                                      unsigned frame_index)
{
    return true;
}

bool VulkanContext::start_render_pass(CommandList* cmd_list,
                                      unsigned rt_count,
                                      const RenderTarget* const* rts,
                                      const RenderTarget* depth_stencil)
{
    return true;
}

void VulkanContext::set_viewports(CommandList* list, unsigned count, const D3D12_VIEWPORT* viewport)
{
}

void VulkanContext::set_scissor_rects(CommandList* list, unsigned count, const D3D12_RECT* scissor_rect)
{
}

void VulkanContext::draw(CommandList* cmd_list, uint32_t vertex_count, uint32_t start_vertex_offset)
{
}

void VulkanContext::draw_indexed(CommandList* cmd_list,
                                 uint32_t index_count,
                                 uint32_t instance_count,
                                 uint32_t start_index_offset,
                                 int32_t base_vertex_offset,
                                 uint32_t instance_offset)
{
}

void VulkanContext::submit_command_lists(const SubmitInfo& submit_info, Queue* queue)
{
}

bool VulkanContext::create_fence(Fence* fence, uint64_t initial_value)
{
    return true;
}

uint64_t VulkanContext::get_fence_value(const Fence& fence)
{
    return 0;
}

bool VulkanContext::wait_fences(const WaitInfo& info)
{
    return true;
}

void VulkanContext::set_barrier_resource(unsigned count,
                                         ImageBarrier* barriers,
                                         const Swapchain& swapchain,
                                         unsigned frame_index)
{
}

void VulkanContext::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Texture& render_target)
{
}

void VulkanContext::issue_barrier(CommandList* cmd_list, unsigned count, const ImageBarrier* barriers)
{
}

void VulkanContext::init_imgui(const DisplayWindow& window, const Swapchain& swapchain)
{
}

void VulkanContext::start_imgui_frame()
{
}

void VulkanContext::render_imgui_draw_data(CommandList* cmd_list)
{
}

void VulkanContext::destroy_imgui()
{
}

bool VulkanContext::compatibility_set_constant_buffers(unsigned slot,
                                                       unsigned count,
                                                       Buffer* const* buffers,
                                                       PipelineStage stage)
{
    return false;
}

bool VulkanContext::compatibility_set_shader_buffers(unsigned slot,
                                                     unsigned count,
                                                     Descriptor* const* descriptors,
                                                     PipelineStage stage)
{
    return false;
}

bool VulkanContext::compatibility_set_uav_buffers(unsigned slot, unsigned count, Buffer* const* buffers)
{
    return false;
}

bool VulkanContext::compatibility_set_textures(
    unsigned slot, unsigned count, Descriptor* const* descriptors, AccessFlags flag, PipelineStage stage)
{
    return false;
}

bool VulkanContext::compatibility_set_samplers(unsigned slot,
                                               unsigned count,
                                               Descriptor* const* samplers,
                                               PipelineStage stage)
{
    return false;
}

bool VulkanContext::wait_idle(Queue* queue)
{
    return true;
}

VulkanContext::~VulkanContext()
{
    vmaDestroyAllocator(m_allocator);
}
