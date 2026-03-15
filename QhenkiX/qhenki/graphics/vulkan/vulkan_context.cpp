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
#include "vulkan_command_pool.h"
#include "vulkan_descriptor_heap.h"
#include "vulkan_root_signature.h"
#include "vulkan_shader.h"
#include "vulkan_texture.h"

#include <spirv_glsl.hpp>

#include "qhenki/utility/string_util.h"
#include "qhenki/utility/vulkan_util.h"

constexpr uint32_t PUSH_RESERVED_START_OFFSET = 128u;
constexpr uint32_t MAX_VERTEX_SLOTS = 32u;

using namespace qhenki::gfx;

namespace
{
VulkanBuffer* to_internal(const Buffer& ext)
{
    const auto vulkan_buffer = static_cast<VulkanBuffer*>(ext.internal_state.get());
    assert(vulkan_buffer);
    return vulkan_buffer;
}

VulkanTexture* to_internal(const Texture& ext)
{
    const auto vulkan_texture = static_cast<VulkanTexture*>(ext.internal_state.get());
    assert(vulkan_texture);
    return vulkan_texture;
}

VulkanRootSignature* to_internal(const PipelineLayout& ext)
{
    const auto vulkan_root_sig = static_cast<VulkanRootSignature*>(ext.internal_state.get());
    assert(vulkan_root_sig);
    return vulkan_root_sig;
}

VkPipeline* to_internal(const GraphicsPipeline& ext)
{
    const auto vulkan_pipeline = static_cast<VkPipeline*>(ext.internal_state.get());
    assert(vulkan_pipeline);
    return vulkan_pipeline;
}

VulkanDescriptorHeap* to_internal(const DescriptorHeap& ext)
{
    const auto vulkan_descriptor_heap = static_cast<VulkanDescriptorHeap*>(ext.internal_state.get());
    assert(vulkan_descriptor_heap);
    return vulkan_descriptor_heap;
}

VkSemaphore* to_internal(const Fence& ext)
{
    const auto vulkan_semaphore = static_cast<VkSemaphore*>(ext.internal_state.get());
    assert(vulkan_semaphore);
    return vulkan_semaphore;
}

VulkanCommandPool* to_internal(const CommandPool& ext)
{
    const auto vulkan_command_pool = static_cast<VulkanCommandPool*>(ext.internal_state.get());
    assert(vulkan_command_pool);
    return vulkan_command_pool;
}

VkCommandBuffer* to_internal(const CommandList& ext)
{
    const auto vulkan_command_buffer = static_cast<VkCommandBuffer*>(ext.internal_state.get());
    assert(vulkan_command_buffer);
    return vulkan_command_buffer;
}

VulkanShader* to_internal(const Shader& ext)
{
    const auto vulkan_shader = static_cast<VulkanShader*>(ext.internal_state.get());
    assert(vulkan_shader);
    return vulkan_shader;
}

void set_debug_name(const VkDevice device, const VkObjectType type, const uint64_t handle, const char* name)
{
    if (vkSetDebugUtilsObjectNameEXT)
    {
        const VkDebugUtilsObjectNameInfoEXT name_info{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = handle,
            .pObjectName = name,
        };
        vkSetDebugUtilsObjectNameEXT(device, &name_info);
    }
}
} // namespace

constexpr uint32_t major = 1;
constexpr uint32_t minor = 4;

std::string VulkanContext::create(const bool enable_debug_layer)
{
    if (VK_FAILED(volkInitialize()))
    {
        return "Vulkan: Failed to initialize Volk";
    }

    vkb::InstanceBuilder builder;

    builder.set_engine_name("QhenkiX").set_engine_version(0, 1, 0);

    uint32_t sdl_ext_count = 0;
    const auto sdl_extensions = SDL_Vulkan_GetInstanceExtensions(&sdl_ext_count);
    builder.enable_extensions(sdl_ext_count, sdl_extensions);

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
    std::array<const char*, 6> device_extensions{
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_9_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
        VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME,
        VK_GOOGLE_USER_TYPE_EXTENSION_NAME,
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
        .vertexPipelineStoresAndAtomics = VK_TRUE,
        .fragmentStoresAndAtomics = VK_TRUE,
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
        .shaderBufferInt64Atomics = VK_TRUE,
        .shaderSharedInt64Atomics = VK_TRUE,
        .descriptorIndexing = VK_TRUE,
        .shaderInputAttachmentArrayDynamicIndexing = VK_TRUE,
        .shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE,
        .shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE,
        .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
        .shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE,
        .shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .scalarBlockLayout = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
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
    m_device = std::move(dev_ret.value());

    auto graphics_queue_result = m_device.get_queue_and_index(vkb::QueueType::graphics);
    if (!graphics_queue_result)
    {
        return "Vulkan: Failed to get graphics queue" + graphics_queue_result.error().message();
    }
    auto [graphics_queue, graphics_queue_index] = graphics_queue_result.value();
    m_graphics_queue = {graphics_queue, graphics_queue_index};

    // Compute queue may equal graphics queue
    auto compute_queue_result = m_device.get_queue_and_index(vkb::QueueType::compute);
    if (!compute_queue_result)
    {
        return "Vulkan: Failed to get compute queue" + compute_queue_result.error().message();
    }
    auto [compute_queue, compute_queue_index] = compute_queue_result.value();
    m_compute_queue = {compute_queue, compute_queue_index};

    // Transfer queue may equal graphics or compute queue
    auto transfer_queue_result = m_device.get_queue_and_index(vkb::QueueType::transfer);
    if (!transfer_queue_result)
    {
        return "Vulkan: Failed to get transfer queue" + transfer_queue_result.error().message();
    }
    auto [transfer_queue, transfer_queue_index] = transfer_queue_result.value();
    m_transfer_queue = {transfer_queue, transfer_queue_index};

    volkLoadDevice(m_device.device);

    VmaVulkanFunctions vulkan_functions{};
    VmaAllocatorCreateInfo allocator_desc{
        .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT |
                 VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT |
                 VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_device.physical_device,
        .device = m_device.device,
        .instance = m_instance.instance,
        .vulkanApiVersion = VK_API_VERSION_1_4,
    };

    if (VK_FAILED(vmaImportVulkanFunctionsFromVolk(&allocator_desc, &vulkan_functions)))
    {
        return "Vulkan: Failed to import Vulkan functions for VMA from Volk";
    }

    allocator_desc.pVulkanFunctions = &vulkan_functions;

    if (VK_FAILED(vmaCreateAllocator(&allocator_desc, &m_allocator)))
    {
        return "Vulkan: Failed to create VMA allocator";
    }

    vkGetPhysicalDeviceProperties2(m_device.physical_device, &m_capabilities.properties);

    const auto& limits = m_capabilities.properties.properties.limits;
    if (limits.maxPushConstantsSize < PUSH_RESERVED_START_OFFSET)
    {
        return "Vulkan: Device does not support required push constant size";
    }
    if (limits.maxColorAttachments < MAX_RENDER_TARGETS)
    {
        return "Vulkan: Device does not support required number of color attachments";
    }
    if (limits.maxVertexInputAttributes < MAX_VERTEX_SLOTS)
    {
        // TODO: Relax this requirement by changing to 16?
        return "Vulkan: Device does not support required number of vertex input attributes";
    }

    return "";
}

bool VulkanContext::is_compatibility() const
{
    return false;
}

bool VulkanContext::create_swapchain(const DisplayWindow& window,
                                     const SwapchainDesc& swapchain_desc,
                                     unsigned* frame_index)
{
    if (!SDL_Vulkan_CreateSurface(window.get_window(), m_instance, nullptr, &m_surface))
    {
        return false;
    }

    vkb::SwapchainBuilder swapchain_builder{m_device, m_surface};

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
    m_swapchain.swapchain = std::move(swap_ret.value());

    auto swap_img = m_swapchain.swapchain.get_images();
    if (!swap_img)
    {
        return false;
    }
    m_swapchain.images = std::move(swap_img.value());

    auto swap_img_view = m_swapchain.swapchain.get_image_views();
    if (!swap_img_view)
    {
        return false;
    }
    m_swapchain.image_views = std::move(swap_img_view.value());

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
    ++m_frame_count;
    return true;
}

unsigned VulkanContext::get_swapchain_frame_index(const Swapchain& swapchain)
{
    return 0;
}

bool VulkanContext::create_shader(void* data, const size_t size, const ShaderType type, Shader* shader)
{
    assert(shader);

    auto state = mkS<VulkanShader>();
    state->spirv.resize(size / sizeof(uint32_t));
    memcpy(state->spirv.data(), data, size);

    const VkShaderModuleCreateInfo shader_module_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = state->spirv.data(),
    };

    if (VK_FAILED(vkCreateShaderModule(m_device.device, &shader_module_info, nullptr, &state->module)))
    {
        state.reset();
        return false;
    }

    shader->type = type;
    shader->internal_state = std::move(state);

    return true;
}

namespace
{
VkFormat vertex_attribute_format_from_spirv_type(const spirv_cross::SPIRType& type)
{
    using Base = spirv_cross::SPIRType::BaseType;

    const auto components = type.vecsize;
    if (type.columns != 1 || components == 0 || components > 4)
    {
        return VK_FORMAT_UNDEFINED;
    }

    switch (type.basetype)
    {
    case Base::Float:
        switch (components)
        {
        case 1:
            return VK_FORMAT_R32_SFLOAT;
        case 2:
            return VK_FORMAT_R32G32_SFLOAT;
        case 3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case 4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        break;
    case Base::Int:
        switch (components)
        {
        case 1:
            return VK_FORMAT_R32_SINT;
        case 2:
            return VK_FORMAT_R32G32_SINT;
        case 3:
            return VK_FORMAT_R32G32B32_SINT;
        case 4:
            return VK_FORMAT_R32G32B32A32_SINT;
        }
        break;
    case Base::UInt:
        switch (components)
        {
        case 1:
            return VK_FORMAT_R32_UINT;
        case 2:
            return VK_FORMAT_R32G32_UINT;
        case 3:
            return VK_FORMAT_R32G32B32_UINT;
        case 4:
            return VK_FORMAT_R32G32B32A32_UINT;
        }
        break;
    default:
        break;
    }

    return VK_FORMAT_UNDEFINED;
}

uint32_t vertex_attribute_size_from_spirv_type(const spirv_cross::SPIRType& type)
{
    return type.width / 8u * type.vecsize;
}
} // namespace

bool VulkanContext::create_pipeline(const GraphicsPipelineDesc& desc,
                                    GraphicsPipeline* pipeline,
                                    const Shader& vertex_shader,
                                    const Shader& pixel_shader,
                                    PipelineLayout* in_layout,
                                    const char* debug_name)
{
    assert(pipeline);
    if (desc.num_render_targets > MAX_RENDER_TARGETS)
    {
        return false;
    }

    const auto vk_vertex_shader = to_internal(vertex_shader);
    const spirv_cross::CompilerGLSL vs_reflect(vk_vertex_shader->spirv);
    spirv_cross::ShaderResources resources = vs_reflect.get_shader_resources();

    const auto vs_entry_name = vs_reflect.get_entry_points_and_stages()[0].name;

    thread_local memory::Arena arena(util::MEGABYTE);
    arena.reset();

    const auto input_attributes = arena.alloc_array<VkVertexInputAttributeDescription>(resources.stage_inputs.size());

    const uint32_t binding_count = desc.increment_slot ? static_cast<uint32_t>(resources.stage_inputs.size()) : 1u;
    const auto binding_descs = arena.alloc_array<VkVertexInputBindingDescription>(binding_count);

    uint32_t offset = 0;
    for (uint32_t i = 0; i < resources.stage_inputs.size(); i++)
    {
        const auto& input = resources.stage_inputs[i];
        const auto& type = vs_reflect.get_type(input.type_id);
        const auto format = vertex_attribute_format_from_spirv_type(type);
        if (format == VK_FORMAT_UNDEFINED)
        {
            return false;
        }

        const uint32_t attrib_size = vertex_attribute_size_from_spirv_type(type);
        const uint32_t binding = desc.increment_slot ? i : 0u;
        const uint32_t attrib_offset = desc.increment_slot ? 0u : offset;

        input_attributes[i] = {
            .location = vs_reflect.get_decoration(input.id, spv::DecorationLocation),
            .binding = binding,
            .format = format,
            .offset = attrib_offset,
        };

        if (desc.increment_slot)
        {
            binding_descs[i] = {
                .binding = binding,
                .stride = attrib_size,
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };
        }
        else
        {
            offset += attrib_size;
        }
    }

    if (!desc.increment_slot)
    {
        binding_descs[0] = {
            .binding = 0,
            .stride = offset,
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    const auto ps_vk_pixel_shader = to_internal(pixel_shader);
    const spirv_cross::CompilerGLSL ps_reflect(ps_vk_pixel_shader->spirv);
    const auto ps_entry_name = ps_reflect.get_entry_points_and_stages()[0].name;

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
    shader_stages[0] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vk_vertex_shader->module,
        .pName = vs_entry_name.c_str(),
    };
    shader_stages[1] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = ps_vk_pixel_shader->module,
        .pName = ps_entry_name.c_str(),
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = binding_count,
        .pVertexBindingDescriptions = binding_descs,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(resources.stage_inputs.size()),
        .pVertexAttributeDescriptions = input_attributes,
    };

    // We require multi-viewport device feature
    VkPipelineViewportStateCreateInfo viewport_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = m_capabilities.properties.properties.limits.maxViewports,
        .scissorCount = m_capabilities.properties.properties.limits.maxViewports,
    };

    std::array dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,
        VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data(),
    };

    const VkPrimitiveTopology primitive_topology = get_primitive_topology(desc.topology);
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = primitive_topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    auto make_vk_rasterizer_state = [](const RasterizerDesc& r)
    {
        VkPolygonMode polygon_mode;
        switch (r.fill_mode)
        {
        case D3D12_FILL_MODE_WIREFRAME:
            polygon_mode = VK_POLYGON_MODE_LINE;
            break;
        case D3D12_FILL_MODE_SOLID:
        default:
            polygon_mode = VK_POLYGON_MODE_FILL;
            break;
        }

        VkCullModeFlags cull_mode;
        switch (r.cull_mode)
        {
        case D3D12_CULL_MODE_FRONT:
            cull_mode = VK_CULL_MODE_FRONT_BIT;
            break;
        case D3D12_CULL_MODE_BACK:
            cull_mode = VK_CULL_MODE_BACK_BIT;
            break;
        case D3D12_CULL_MODE_NONE:
        default:
            cull_mode = VK_CULL_MODE_NONE;
            break;
        }

        const VkPipelineRasterizationStateCreateInfo raster{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            // TODO: Clamp instead of clip?
            .depthClampEnable = r.depth_clip_enable ? VK_FALSE : VK_TRUE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = polygon_mode,
            .cullMode = cull_mode,
            .frontFace = r.front_counter_clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = r.depth_bias != 0 || r.slope_scaled_depth_bias != 0.0f ? VK_TRUE : VK_FALSE,
            .depthBiasConstantFactor = static_cast<float>(r.depth_bias),
            .depthBiasClamp = r.depth_bias_clamp,
            .depthBiasSlopeFactor = r.slope_scaled_depth_bias,
            .lineWidth = 1.0f,
        };
        return raster;
    };

    constexpr RasterizerDesc default_rasterizer{};
    const auto& raster_desc = desc.rasterizer_state.value_or(default_rasterizer);
    VkPipelineRasterizationStateCreateInfo rasterization_info = make_vk_rasterizer_state(raster_desc);

    VkBool32 alpha_to_coverage = VK_FALSE;
    if (desc.blend_desc.has_value())
    {
        alpha_to_coverage = desc.blend_desc->AlphaToCoverageEnable ? VK_TRUE : VK_FALSE;
    }

    // TODO: Replace with enum
    assert(util::is_power_of_two(desc.sample_count) && desc.sample_count <= VK_SAMPLE_COUNT_64_BIT);
    VkPipelineMultisampleStateCreateInfo multisample_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = static_cast<VkSampleCountFlagBits>(desc.sample_count),
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = alpha_to_coverage,
        .alphaToOneEnable = VK_FALSE,
    };

    auto map_stencil_op = [](const D3D12_STENCIL_OP op)
    {
        switch (op)
        {
        case D3D12_STENCIL_OP_KEEP:
            return VK_STENCIL_OP_KEEP;
        case D3D12_STENCIL_OP_ZERO:
            return VK_STENCIL_OP_ZERO;
        case D3D12_STENCIL_OP_REPLACE:
            return VK_STENCIL_OP_REPLACE;
        case D3D12_STENCIL_OP_INCR_SAT:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case D3D12_STENCIL_OP_DECR_SAT:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case D3D12_STENCIL_OP_INVERT:
            return VK_STENCIL_OP_INVERT;
        case D3D12_STENCIL_OP_INCR:
            return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case D3D12_STENCIL_OP_DECR:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        default:
            return VK_STENCIL_OP_KEEP;
        }
    };

    auto map_compare_func = [](const D3D12_COMPARISON_FUNC func)
    {
        switch (func)
        {
        case D3D12_COMPARISON_FUNC_NEVER:
            return VK_COMPARE_OP_NEVER;
        case D3D12_COMPARISON_FUNC_LESS:
            return VK_COMPARE_OP_LESS;
        case D3D12_COMPARISON_FUNC_EQUAL:
            return VK_COMPARE_OP_EQUAL;
        case D3D12_COMPARISON_FUNC_LESS_EQUAL:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case D3D12_COMPARISON_FUNC_GREATER:
            return VK_COMPARE_OP_GREATER;
        case D3D12_COMPARISON_FUNC_NOT_EQUAL:
            return VK_COMPARE_OP_NOT_EQUAL;
        case D3D12_COMPARISON_FUNC_GREATER_EQUAL:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case D3D12_COMPARISON_FUNC_ALWAYS:
        default:
            return VK_COMPARE_OP_ALWAYS;
        }
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    const bool has_depth_stencil_desc = desc.depth_stencil_state.has_value();
    VkFormat depth_stencil_format = VK_FORMAT_UNDEFINED;
    if (desc.dsv_format != DXGI_FORMAT_UNKNOWN)
    {
        depth_stencil_format = convert_format(desc.dsv_format);
    }

    const bool has_depth_attachment = has_depth_stencil_desc && depth_stencil_format != VK_FORMAT_UNDEFINED &&
                                      is_depth_stencil_format(depth_stencil_format);

    if (has_depth_attachment)
    {
        const auto& ds = *desc.depth_stencil_state;

        const auto make_stencil_state = [&](const D3D12_DEPTH_STENCILOP_DESC& op)
        {
            VkStencilOpState state;
            state.failOp = map_stencil_op(op.StencilFailOp);
            state.passOp = map_stencil_op(op.StencilPassOp);
            state.depthFailOp = map_stencil_op(op.StencilDepthFailOp);
            state.compareOp = map_compare_func(ds.depth_func);
            state.compareMask = ds.stencil_read_mask;
            state.writeMask = ds.stencil_write_mask;
            state.reference = 0;
            return state;
        };

        depth_stencil_info = {
            .depthTestEnable = ds.depth_enable ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = ds.depth_write_mask == D3D12_DEPTH_WRITE_MASK_ZERO ? VK_FALSE : VK_TRUE,
            .depthCompareOp = map_compare_func(ds.depth_func),
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = ds.stencil_enable ? VK_TRUE : VK_FALSE,
            .front = make_stencil_state(ds.front_face),
            .back = make_stencil_state(ds.back_face),
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        };
    }

    // Color blend state
    auto map_blend_factor = [](const D3D12_BLEND b)
    {
        switch (b)
        {
        case D3D12_BLEND_ZERO:
            return VK_BLEND_FACTOR_ZERO;
        case D3D12_BLEND_ONE:
            return VK_BLEND_FACTOR_ONE;
        case D3D12_BLEND_SRC_COLOR:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case D3D12_BLEND_INV_SRC_COLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case D3D12_BLEND_SRC_ALPHA:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case D3D12_BLEND_INV_SRC_ALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case D3D12_BLEND_DEST_ALPHA:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case D3D12_BLEND_INV_DEST_ALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case D3D12_BLEND_DEST_COLOR:
            return VK_BLEND_FACTOR_DST_COLOR;
        case D3D12_BLEND_INV_DEST_COLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case D3D12_BLEND_SRC_ALPHA_SAT:
            return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
        case D3D12_BLEND_BLEND_FACTOR:
            return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case D3D12_BLEND_INV_BLEND_FACTOR:
            return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case D3D12_BLEND_SRC1_COLOR:
            return VK_BLEND_FACTOR_SRC1_COLOR;
        case D3D12_BLEND_INV_SRC1_COLOR:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
        case D3D12_BLEND_SRC1_ALPHA:
            return VK_BLEND_FACTOR_SRC1_ALPHA;
        case D3D12_BLEND_INV_SRC1_ALPHA:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
        default:
            return VK_BLEND_FACTOR_ONE;
        }
    };

    auto map_blend_op = [](const D3D12_BLEND_OP op)
    {
        switch (op)
        {
        case D3D12_BLEND_OP_ADD:
            return VK_BLEND_OP_ADD;
        case D3D12_BLEND_OP_SUBTRACT:
            return VK_BLEND_OP_SUBTRACT;
        case D3D12_BLEND_OP_REV_SUBTRACT:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case D3D12_BLEND_OP_MIN:
            return VK_BLEND_OP_MIN;
        case D3D12_BLEND_OP_MAX:
            return VK_BLEND_OP_MAX;
        default:
            return VK_BLEND_OP_ADD;
        }
    };

    auto map_color_write_mask = [](const UINT8 mask)
    {
        VkColorComponentFlags flags = 0;
        if (mask & D3D12_COLOR_WRITE_ENABLE_RED)
        {
            flags |= VK_COLOR_COMPONENT_R_BIT;
        }
        if (mask & D3D12_COLOR_WRITE_ENABLE_GREEN)
        {
            flags |= VK_COLOR_COMPONENT_G_BIT;
        }
        if (mask & D3D12_COLOR_WRITE_ENABLE_BLUE)
        {
            flags |= VK_COLOR_COMPONENT_B_BIT;
        }
        if (mask & D3D12_COLOR_WRITE_ENABLE_ALPHA)
        {
            flags |= VK_COLOR_COMPONENT_A_BIT;
        }
        return flags;
    };

    std::array<VkPipelineColorBlendAttachmentState, MAX_RENDER_TARGETS> color_attachments{};
    for (unsigned i = 0; i < desc.num_render_targets; i++)
    {
        VkPipelineColorBlendAttachmentState attachment{};
        if (desc.blend_desc.has_value())
        {
            const auto& rt = desc.blend_desc->RenderTarget[i];
            attachment.blendEnable = rt.BlendEnable ? VK_TRUE : VK_FALSE;
            attachment.srcColorBlendFactor = map_blend_factor(rt.SrcBlend);
            attachment.dstColorBlendFactor = map_blend_factor(rt.DestBlend);
            attachment.colorBlendOp = map_blend_op(rt.BlendOp);
            attachment.srcAlphaBlendFactor = map_blend_factor(rt.SrcBlendAlpha);
            attachment.dstAlphaBlendFactor = map_blend_factor(rt.DestBlendAlpha);
            attachment.alphaBlendOp = map_blend_op(rt.BlendOpAlpha);
            attachment.colorWriteMask = map_color_write_mask(rt.RenderTargetWriteMask);
        }
        else
        {
            attachment.blendEnable = VK_FALSE;
            attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.colorBlendOp = VK_BLEND_OP_ADD;
            attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            attachment.alphaBlendOp = VK_BLEND_OP_ADD;
            attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
                                        VK_COLOR_COMPONENT_A_BIT;
        }

        color_attachments[i] = attachment;
    }

    VkPipelineColorBlendStateCreateInfo color_blend_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = desc.num_render_targets,
        .pAttachments = color_attachments.data(),
    };

    std::array<VkFormat, 8> color_formats{};
    for (unsigned i = 0; i < desc.num_render_targets; i++)
    {
        color_formats[i] = convert_format(desc.rtv_formats[i]);
    }

    VkPipelineCreateFlags2CreateInfo flags{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
        .flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT,
    };

    VkPipelineRenderingCreateInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext = &flags,
        .colorAttachmentCount = desc.num_render_targets,
        .pColorAttachmentFormats = desc.num_render_targets ? color_formats.data() : nullptr,
        .depthAttachmentFormat = has_depth_attachment ? depth_stencil_format : VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = has_depth_attachment && desc.depth_stencil_state->stencil_enable
                                     ? depth_stencil_format
                                     : VK_FORMAT_UNDEFINED,
    };

    VkGraphicsPipelineCreateInfo pipeline_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_info,
        .stageCount = shader_stages.size(),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly_info,
        .pViewportState = &viewport_state_info,
        .pRasterizationState = &rasterization_info,
        .pMultisampleState = &multisample_info,
        .pDepthStencilState = &depth_stencil_info,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = &dynamic_state_info,
    };

    VkPipeline vk_pipeline;
    if (VK_FAILED(vkCreateGraphicsPipelines(m_device.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &vk_pipeline)))
    {
        return false;
    }

    pipeline->internal_state = mkS<VkPipeline>(vk_pipeline);

    set_debug_name(m_device.device, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(vk_pipeline), debug_name);

    return true;
}

bool VulkanContext::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    const auto vk_pipeline = to_internal(pipeline);
    vkCmdBindPipeline(*vk_cmd_list, VK_PIPELINE_BIND_POINT_GRAPHICS, *vk_pipeline);
    return true;
}

bool VulkanContext::create_pipeline_layout(PipelineLayoutDesc* desc, PipelineLayout* layout)
{
    assert(layout);

    assert(desc->spaces.size() + desc->push_ranges.size() <= MAX_SPACES);
    // This should never fire since we target Vulkan 1.4
    assert(m_capabilities.descriptor_heap_properties.maxPushDataSize >= 256);

    constexpr VkSpirvResourceTypeFlagsEXT srv_mask = VK_SPIRV_RESOURCE_TYPE_SAMPLED_IMAGE_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_READ_ONLY_IMAGE_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_READ_ONLY_STORAGE_BUFFER_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_ACCELERATION_STRUCTURE_BIT_EXT;
    constexpr VkSpirvResourceTypeFlagsEXT sampler_mask = VK_SPIRV_RESOURCE_TYPE_SAMPLER_BIT_EXT;
    constexpr VkSpirvResourceTypeFlagsEXT uav_mask = VK_SPIRV_RESOURCE_TYPE_READ_WRITE_IMAGE_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_READ_WRITE_STORAGE_BUFFER_BIT_EXT;
    constexpr VkSpirvResourceTypeFlagsEXT cbv_mask = VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT;

    layout->internal_state = mkS<VulkanRootSignature>();

    const auto vk_root_signature = to_internal(*layout);

    auto params = vk_root_signature->bindings;

    uint32_t push_offset = 0;
    for (unsigned i = 0; i < desc->push_ranges.size(); i++)
    {
        const auto& range = desc->push_ranges[i];
        if (range.size % 4u != 0)
        {
            params.clear();
            return false;
        }
        params.push_back({
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
            .descriptorSet = range.space,
            .firstBinding = range.binding,
            .bindingCount = 1,
            .resourceMask = cbv_mask,
            .source = VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_DATA_EXT,
            .sourceData = {.pushDataOffset = push_offset},
        });
        push_offset += util::ceil_div(range.size, 4u);
        assert(push_offset < PUSH_RESERVED_START_OFFSET);
    }

    push_offset = PUSH_RESERVED_START_OFFSET;

    for (unsigned i = 0; i < desc->spaces.size(); i++)
    {
        auto& space = desc->spaces[i];
        if (space.empty())
        {
            continue;
        }
        // Sort vector of LayoutBindings by binding register
        std::ranges::sort(space,
                          [](const LayoutBinding& a, const LayoutBinding& b)
                          {
                              return a.binding < b.binding;
                          });
        uint32_t heap_offset = 0;

        for (unsigned j = 0; j < space.size(); j++)
        {
            const auto& binding = space[j];
            // Check that this is not the last binding and not infinite register count
            assert(j == space.size() - 1 || binding.count != INFINITE_DESCRIPTORS);

            VkSpirvResourceTypeFlagsEXT mask = 0;
            uint32_t descriptor_size = get_descriptor_size(Descriptor::Type::BUFFER);
            switch (binding.type)
            {
            case LayoutBinding::RangeType::SRV_BUFFER:
            case LayoutBinding::RangeType::SRV_TEXTURE:
                mask = srv_mask;
                descriptor_size = get_descriptor_size(Descriptor::Type::TEXTURE);
                break;
            case LayoutBinding::RangeType::UAV_BUFFER:
            case LayoutBinding::RangeType::UAV_TEXTURE:
                mask = uav_mask;
                descriptor_size = get_descriptor_size(Descriptor::Type::TEXTURE);
                break;
            case LayoutBinding::RangeType::CBV:
                mask = cbv_mask;
                break;
            case LayoutBinding::RangeType::SAMPLER:
                mask = sampler_mask;
                descriptor_size = get_descriptor_size(Descriptor::Type::SAMPLER);
                break;
            }
            assert(mask);
            assert(descriptor_size);

            params.push_back({
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
                .descriptorSet = i,
                .firstBinding = binding.binding,
                .bindingCount = binding.count,
                .resourceMask = mask,
                .source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_PUSH_INDEX_EXT,
                .sourceData =
                    {
                        .pushIndex{
                            .heapOffset = heap_offset,
                            // We rely on 256 byte minimum push constant size, second half is used for internal logic
                            .pushOffset = push_offset,
                            .heapIndexStride = 1, // Byte scaling for pushOffset
                        },
                    },
            });

            heap_offset += binding.count * descriptor_size;
        }
        push_offset += sizeof(uint32_t);

        assert(push_offset < m_capabilities.descriptor_heap_properties.maxPushDataSize);
    }

    vk_root_signature->layout = {
        .sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT,
        .mappingCount = static_cast<uint32_t>(params.size()),
        .pMappings = params.data(),
    };

    return true;
}

void VulkanContext::bind_pipeline_layout(CommandList* cmd_list, const PipelineLayout& layout)
{
    // TODO: Delete this function and have it happen in pipeline binding for D3D12
}

bool VulkanContext::set_pipeline_constant(
    CommandList* cmd_list, unsigned param, const uint32_t offset, const unsigned size, void* data)
{
    if (offset % 4u != 0)
    {
        return false;
    }
    if (size % 4u != 0)
    {
        return false;
    }

    const auto vk_cmd_list = to_internal(*cmd_list);
    const VkPushDataInfoEXT push_data_info{.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
                                           .offset = 0 + offset, // TODO: Use param to calculate offset
                                           .data = {.address = data, .size = size}};
    assert(push_data_info.offset % 4u == 0);
    vkCmdPushDataEXT(*vk_cmd_list, &push_data_info);

    return true;
}

bool VulkanContext::create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap* heap, const char* debug_name)
{
    heap->internal_state = mkS<VulkanDescriptorHeap>();
    const auto vk_heap = to_internal(*heap);

    if (!vk_heap->create(desc, *this))
    {
        heap->internal_state.reset();
        return false;
    }

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
    const auto vk_heap = to_internal(heap);

    const VkDeviceAddressRangeEXT heap_range{
        .address = vk_heap->get_address(),
        .size = heap.desc.size,
    };

    const VkBindHeapInfoEXT bind_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange = heap_range,
        .reservedRangeOffset = 0,
        .reservedRangeSize = vk_heap->get_reserved_size(),
    };

    const auto vk_cmd_list = to_internal(*cmd_list);

    vkCmdBindResourceHeapEXT(*vk_cmd_list, &bind_heap_info);
}

void VulkanContext::set_descriptor_heap(CommandList* cmd_list,
                                        const DescriptorHeap& heap,
                                        const DescriptorHeap& sampler_heap)
{
    const auto vk_heap = to_internal(heap);
    const auto vk_sampler_heap = to_internal(sampler_heap);

    const VkDeviceAddressRangeEXT heap_range{
        .address = vk_heap->get_address(),
        .size = heap.desc.size,
    };

    const VkBindHeapInfoEXT bind_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange = heap_range,
        .reservedRangeOffset = 0,
        .reservedRangeSize = vk_heap->get_reserved_size(),
    };

    const VkDeviceAddressRangeEXT sampler_heap_range{
        .address = vk_sampler_heap->get_address(),
        .size = sampler_heap.desc.size,
    };

    const VkBindHeapInfoEXT sampler_bind_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange = sampler_heap_range,
        .reservedRangeOffset = 0,
        .reservedRangeSize = vk_sampler_heap->get_reserved_size(),
    };

    const auto vk_cmd_list = to_internal(*cmd_list);

    vkCmdBindResourceHeapEXT(*vk_cmd_list, &bind_heap_info);
    vkCmdBindSamplerHeapEXT(*vk_cmd_list, &sampler_bind_heap_info);
}

void VulkanContext::set_descriptor_table(CommandList* cmd_list, const unsigned index, const Descriptor& gpu_descriptor)
{
    const auto vk_cmd_list = to_internal(*cmd_list);

    uint32_t data = 0;
    const VkPushDataInfoEXT push_data_info{
        .sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .offset = static_cast<uint32_t>(PUSH_RESERVED_START_OFFSET + index * sizeof(size_t)), // Implies max 16 params
        .data = {.address = &gpu_descriptor.offset, .size = sizeof(size_t)}};
    vkCmdPushDataEXT(*vk_cmd_list, &push_data_info);
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
    assert(buffer);

    buffer->internal_state = mkS<VulkanBuffer>();
    const auto vulkan_buffer = static_cast<VulkanBuffer*>(buffer->internal_state.get());

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = desc.size,
    };

    buffer_info.usage = 0;
    if (desc.usage & BufferUsage::VERTEX)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::INDEX)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::CONSTANT)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    // BufferUsage::SHADER doesn't have any meaning here
    if (desc.usage & BufferUsage::UAV)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::INDIRECT)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if (desc.usage & BufferUsage::COPY_SRC)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc.usage & BufferUsage::COPY_DST)
    {
        buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    assert(buffer_info.usage);

    VmaAllocationCreateInfo alloc_info = {VMA_MEMORY_USAGE_UNKNOWN};
    const auto is_cpu_visible = desc.visibility & CPU_SEQUENTIAL;
    if (is_cpu_visible)
    {
        constexpr auto cpu = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        alloc_info.requiredFlags = desc.visibility & GPU ? cpu | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : cpu;
    }
    else
    {
        alloc_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    if (VK_FAILED(vmaCreateBuffer(
            m_allocator, &buffer_info, &alloc_info, &vulkan_buffer->buffer, &vulkan_buffer->allocation, nullptr)))
    {
        buffer->internal_state.reset();
        return false;
    }

    if (data)
    {
        if (is_cpu_visible)
        {
            void* mapped;
            if (VK_FAILED(vmaMapMemory(m_allocator, vulkan_buffer->allocation, &mapped)))
            {
                buffer->internal_state.reset();
                return false;
            }
            memcpy(mapped, data, desc.size);
            vmaUnmapMemory(m_allocator, vulkan_buffer->allocation);
        }
    }

    set_debug_name(m_device.device,
                   VK_OBJECT_TYPE_BUFFER,
                   reinterpret_cast<uint64_t>(vulkan_buffer->buffer),
                   debug_name);

    vulkan_buffer->allocator = m_allocator;
    buffer->desc = desc;

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
    assert(texture);
    if (desc.height > 1 && desc.dimension == TextureDimension::TEXTURE_1D)
    {
        return false;
    }
    if (desc.usage == TextureDesc::NONE)
    {
        return false;
    }

    VkImageType image_type = VK_IMAGE_TYPE_MAX_ENUM;
    switch (desc.dimension)
    {
    case TextureDimension::TEXTURE_1D:
        image_type = VK_IMAGE_TYPE_1D;
        break;
    case TextureDimension::TEXTURE_2D:
        image_type = VK_IMAGE_TYPE_2D;
        break;
    case TextureDimension::TEXTURE_3D:
        image_type = VK_IMAGE_TYPE_3D;
        break;
    }
    assert(image_type < VK_IMAGE_TYPE_MAX_ENUM);

    assert(desc.width < std::numeric_limits<uint32_t>::max());

    const auto is_3D = desc.dimension == TextureDimension::TEXTURE_3D;

    assert(util::is_power_of_two(desc.sample_count) && desc.sample_count <= 64);
    const VkFormat vk_format = convert_format(desc.format);

    VkImageUsageFlags usage = 0;
    if (desc.usage & TextureDesc::COPY_SOURCE)
    {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (desc.usage & TextureDesc::COPY_DEST)
    {
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (desc.usage & TextureDesc::SHADER_RESOURCE)
    {
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.usage & TextureDesc::UNORDERED_ACCESS)
    {
        usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.usage & TextureDesc::RENDER_TARGET)
    {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (desc.usage & TextureDesc::DEPTH_STENCIL)
    {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (desc.usage & TextureDesc::INPUT_ATTACHMENT)
    {
        usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }
    if (desc.usage & TextureDesc::SHADING_RATE)
    {
        usage |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
    }
    if (desc.usage & TextureDesc::VIDEO_DECODE)
    {
        usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
    }
    if (desc.usage & TextureDesc::VIDEO_ENCODE)
    {
        usage |= VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR | VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR;
    }

    assert(usage);

    const std::array queue_families = {
        m_graphics_queue.family_index,
        m_compute_queue.family_index,
        m_transfer_queue.family_index,
    };

    // Deduplicate queue indices
    std::array<uint32_t, 3> unique_families{};
    uint32_t unique_count = 0;
    for (uint32_t i = 0; i < static_cast<uint32_t>(queue_families.size()); i++)
    {
        const uint32_t idx = queue_families[i];
        bool seen = false;
        for (uint32_t j = 0; j < unique_count; j++)
        {
            if (unique_families[j] == idx)
            {
                seen = true;
                break;
            }
        }
        if (!seen)
        {
            unique_families[unique_count++] = idx;
        }
    }

    VkImageCreateInfo texture_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = image_type,
        .format = vk_format,
        .extent = {static_cast<uint32_t>(desc.width), desc.height, is_3D ? desc.depth_or_array_size : 1u},
        .mipLevels = desc.mip_levels,
        .arrayLayers = is_3D ? 1u : desc.depth_or_array_size,
        .samples = static_cast<VkSampleCountFlagBits>(desc.sample_count),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        // Concurrent sharing is fine on PC and removes need to transfer ownership
        .sharingMode = unique_count > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = unique_count,
        .pQueueFamilyIndices = unique_families.data(),
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    if (desc.is_cube)
    {
        // Cubemaps are 2D arrays with 6 faces per cube
        assert(!is_3D);
        assert(desc.width == desc.height);
        assert(desc.depth_or_array_size % 6 == 0);
        texture_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    constexpr VmaAllocationCreateInfo alloc_info{VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE};

    texture->internal_state = mkS<VulkanTexture>();
    const auto vulkan_texture = static_cast<VulkanTexture*>(texture->internal_state.get());

    if (VK_FAILED(vmaCreateImage(
            m_allocator, &texture_info, &alloc_info, &vulkan_texture->image, &vulkan_texture->allocation, nullptr)))
    {
        texture->internal_state.reset();
        return false;
    }

    vulkan_texture->allocator = m_allocator;
    // TODO: Lazily transition this when touched in command list
    vulkan_texture->initial_layout = layout(desc.initial_layout);
    texture->desc = desc;

    set_debug_name(m_device.device,
                   VK_OBJECT_TYPE_IMAGE,
                   reinterpret_cast<uint64_t>(vulkan_texture->image),
                   debug_name);


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
    const auto vk_buffer = to_internal(buffer);
    void* ptr;
    vmaMapMemory(m_allocator, vk_buffer->allocation, &ptr);
    return ptr;
}

void VulkanContext::unmap_buffer(const Buffer& buffer)
{
    const auto vk_buffer = to_internal(buffer);
    vmaUnmapMemory(m_allocator, vk_buffer->allocation);
}

bool VulkanContext::bind_vertex_buffers(CommandList* cmd_list,
                                        const unsigned start_slot,
                                        const unsigned buffer_count,
                                        const Buffer* const* buffers,
                                        const uint64_t* sizes,
                                        const uint64_t* strides,
                                        const uint64_t* offsets)
{
    if (buffer_count > MAX_VERTEX_SLOTS)
    {
        return false;
    }
    const auto vk_cmd_list = to_internal(*cmd_list);

    std::array<VkBuffer, MAX_VERTEX_SLOTS> vk_buffers;
    for (unsigned i = 0; i < buffer_count; i++)
    {
        const auto vk_buffer = to_internal(*buffers[i]);
        vk_buffers[i] = vk_buffer->buffer;
    }

    vkCmdBindVertexBuffers2(*vk_cmd_list, start_slot, buffer_count, vk_buffers.data(), offsets, sizes, strides);
    return true;
}

void VulkanContext::bind_index_buffer(CommandList* cmd_list,
                                      const Buffer& buffer,
                                      const IndexType format,
                                      const uint64_t offset)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    const auto vk_buffer = to_internal(buffer);

    vkCmdBindIndexBuffer2(*vk_cmd_list,
                          vk_buffer->buffer,
                          offset,
                          buffer.desc.size,
                          format == IndexType::UINT32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

bool VulkanContext::create_command_pool(CommandPool* command_pool, const QueueType queue)
{
    assert(command_pool);
    command_pool->queue_type = queue;

    uint32_t queue_index = 0;
    switch (queue)
    {
    case GRAPHICS:
        queue_index = m_graphics_queue.family_index;
        break;
    case COMPUTE:
        queue_index = m_compute_queue.family_index;
        break;
    case COPY:
        queue_index = m_transfer_queue.family_index;
        break;
    }

    const VkCommandPoolCreateInfo pool_info{.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                            .queueFamilyIndex = queue_index};

    VkCommandPool pool;
    if (VK_FAILED(vkCreateCommandPool(m_device.device, &pool_info, nullptr, &pool)))
    {
        return false;
    }

    // TODO: Stop using RAII
    *command_pool = {
        .queue_type = queue,
        .internal_state = mkS<VulkanCommandPool>(m_device.device, pool),
    };

    return true;
}

bool VulkanContext::create_command_list(CommandList* cmd_list, const CommandPool& command_pool, const char* debug_name)
{
    const auto vk_pool = to_internal(command_pool);
    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    auto cmd_buffer = vk_pool->create_command_buffer(alloc_info);
    if (!cmd_buffer)
    {
        return false;
    }
    cmd_list->internal_state = mkS<VkCommandBuffer>(cmd_buffer);

    return true;
}

bool VulkanContext::reset_command_list(CommandList* cmd_list, const CommandPool& command_pool)
{
    const auto vk_cmd_list = to_internal(*cmd_list);

    // TODO: Somehow keep same debug name
    if (!create_command_list(cmd_list, command_pool, nullptr))
    {
        return false;
    }

    constexpr VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    if (VK_FAILED(vkBeginCommandBuffer(*vk_cmd_list, &begin_info)))
    {
        return false;
    }

    return true;
}

bool VulkanContext::close_command_list(CommandList* cmd_list)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    return VK_SUCCEEDED(vkEndCommandBuffer(*vk_cmd_list));
}

bool VulkanContext::reset_command_pool(CommandPool* command_pool)
{
    const auto vk_pool = to_internal(*command_pool);
    return VK_SUCCEEDED(vk_pool->reset());
}

struct RenderTargetState
{
    std::array<VkImageView, MAX_RENDER_TARGETS> color_render_targets{};
    VkImageView depth_stencil = VK_NULL_HANDLE;
};

namespace
{
RenderTargetState& get_render_target_state(const RenderTarget* const* rts, const RenderTarget* depth_stencil)
{
    thread_local RenderTargetState state;
    return state;
}

bool create_views(const VkDevice device,
                  const unsigned count,
                  const Texture* targets,
                  const Texture* depth_stencil,
                  RenderTargetState* state)
{
    const auto view_type_from_desc = [](const TextureDesc& desc)
    {
        const uint32_t array_layers = desc.depth_or_array_size;
        switch (desc.dimension)
        {
        case TextureDimension::TEXTURE_1D:
            return array_layers > 1 ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        case TextureDimension::TEXTURE_2D:
            if (desc.is_cube)
            {
                return array_layers > 6 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
            }
            return array_layers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        case TextureDimension::TEXTURE_3D:
            return VK_IMAGE_VIEW_TYPE_3D;
        }
        return VK_IMAGE_VIEW_TYPE_2D;
    };

    for (unsigned i = 0; i < count; i++)
    {
        assert(targets[i].desc.usage & TextureDesc::RENDER_TARGET);
        const auto vk_texture = to_internal(targets[i]);
        const VkImageViewCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vk_texture->image,
            .viewType = view_type_from_desc(targets[i].desc),
            .format = convert_format(targets[i].desc.format),
            .subresourceRange = // TODO: Specific mips
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        if (state->color_render_targets[i])
        {
            vkDestroyImageView(device, state->color_render_targets[i], nullptr);
        }
        if (VK_FAILED(vkCreateImageView(device, &info, nullptr, &state->color_render_targets[i])))
        {
            return false;
        }
    }
    if (depth_stencil)
    {
        const auto vk_texture = to_internal(*depth_stencil);
        assert(depth_stencil->desc.usage & TextureDesc::DEPTH_STENCIL);

        VkImageAspectFlags flags;
        switch (depth_stencil->desc.format)
        {
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D32_SFLOAT:
        default:
            flags = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        }

        const VkImageViewCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vk_texture->image,
            .viewType = view_type_from_desc(depth_stencil->desc),
            .format = convert_format(depth_stencil->desc.format),
            .subresourceRange = // TODO: Specific mips
            {
                .aspectMask = flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        if (state->depth_stencil)
        {
            vkDestroyImageView(device, state->depth_stencil, nullptr);
        }
        if (VK_FAILED(vkCreateImageView(device, &info, nullptr, &state->depth_stencil)))
        {
            return false;
        }
    }

    return true;
}
} // namespace

bool VulkanContext::start_render_pass(CommandList* cmd_list,
                                      const float* clear_color_values,
                                      const RenderTarget* depth_stencil,
                                      const unsigned frame_index)
{
    VkRenderingAttachmentInfo color_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_swapchain.image_views[frame_index],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue =
            {
                .color = {clear_color_values[0], clear_color_values[1], clear_color_values[2], clear_color_values[3]},
            },
    };

    VkRenderingAttachmentInfo depth_attachment;

    const auto& swapchain = m_swapchain.swapchain;

    auto& rt_state = get_render_target_state(nullptr, depth_stencil);
    create_views(m_device.device, 0, nullptr, depth_stencil->texture, &rt_state);

    VkExtent2D extent;
    if (depth_stencil)
    {
        assert(depth_stencil->clear_type & RenderTarget::DEPTH || depth_stencil->clear_type & RenderTarget::STENCIL);
        extent = {std::min(static_cast<uint32_t>(depth_stencil->texture->desc.width), swapchain.extent.width),
                  std::min(depth_stencil->texture->desc.height, swapchain.extent.height)};

        const auto& clear_params = depth_stencil->clear_params.dsv_clear_params;
        depth_attachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                            .imageView = rt_state.depth_stencil,
                            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                            .clearValue = {
                                .depthStencil = {clear_params.clear_depth_value, clear_params.clear_stencil_value},
                            }};
    }
    else
    {
        extent = swapchain.extent;
    }

    const VkRect2D render_area{
        .offset = {0, 0},
        .extent = extent,
    };

    const VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = render_area,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = depth_stencil ? &depth_attachment : nullptr,
    };

    const auto vk_cmd_list = to_internal(*cmd_list);
    vkCmdBeginRendering(*vk_cmd_list, &rendering_info);

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

void VulkanContext::submit_command_lists(const SubmitInfo& submit_info, const QueueType queue)
{
}

bool VulkanContext::create_fence(Fence* fence, const uint64_t initial_value)
{
    const VkSemaphoreTypeCreateInfo type_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = initial_value,
    };
    VkSemaphoreCreateInfo semaphore_info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    semaphore_info.pNext = &type_info;

    // TODO: Stop using RAII
    fence->internal_state = mkS<VkSemaphore>();
    const auto vk_fence = to_internal(*fence);

    if (VK_FAILED(vkCreateSemaphore(m_device.device, &semaphore_info, nullptr, vk_fence)))
    {
        fence->internal_state.reset();
        return false;
    }

    return true;
}

uint64_t VulkanContext::get_fence_value(const Fence& fence)
{
    uint64_t value;
    if (VK_FAILED(vkGetSemaphoreCounterValue(m_device.device, *to_internal(fence), &value)))
    {
        return 0;
    }
    return value;
}

bool VulkanContext::wait_fences(const WaitInfo& info)
{
    auto& arena = acquire_arena(m_frame_count);
    const auto semaphores = arena.alloc_array<VkSemaphore>(info.count);

    for (unsigned i = 0; i < info.count; i++)
    {
        semaphores[i] = *to_internal(info.fences[i]);
    }

    const VkSemaphoreWaitInfo wait_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = info.wait_all ? 0u : VK_SEMAPHORE_WAIT_ANY_BIT,
        .semaphoreCount = info.count,
        .pSemaphores = semaphores,
        .pValues = info.values,
    };
    return VK_SUCCEEDED(vkWaitSemaphores(m_device.device, &wait_info, std::numeric_limits<uint64_t>::max()));
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

bool VulkanContext::wait_idle(const QueueType queue)
{
    return true;
}

VulkanContext::~VulkanContext()
{
    vmaDestroyAllocator(m_allocator);
    vkb::destroy_swapchain(m_swapchain.swapchain);
    if (m_surface)
    {
        vkb::destroy_surface(m_instance, m_surface);
    }
    vkb::destroy_device(m_device);
    vkb::destroy_instance(m_instance);
}
