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
#include "vulkan_texture.h"

#include "qhenki/utility/string_util.h"
#include "qhenki/utility/vulkan_util.h"

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

VkShaderModule* to_internal(const Shader& ext)
{
    const auto vulkan_shader_module = static_cast<VkShaderModule*>(ext.internal_state.get());
    assert(vulkan_shader_module);
    return vulkan_shader_module;
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

bool VulkanContext::create_shader(void* data, const size_t size, ShaderType type, Shader* shader)
{
    VkShaderModule shader_module;

    const VkShaderModuleCreateInfo shader_module_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = static_cast<const uint32_t*>(data),
    };

    if (VK_FAILED(vkCreateShaderModule(m_device.device, &shader_module_info, nullptr, &shader_module)))
    {
        return false;
    }

    *shader = {
        .type = type,
        .internal_state = mkS<VkShaderModule>(shader_module),
    };

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
    assert(layout);

    assert(desc->spaces.size() + desc->push_ranges.size() <= MAX_SPACES);

    constexpr VkSpirvResourceTypeFlagsEXT srv_mask = VK_SPIRV_RESOURCE_TYPE_SAMPLED_IMAGE_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_READ_ONLY_IMAGE_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_READ_ONLY_STORAGE_BUFFER_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_ACCELERATION_STRUCTURE_BIT_EXT;
    constexpr VkSpirvResourceTypeFlagsEXT sampler_mask = VK_SPIRV_RESOURCE_TYPE_SAMPLER_BIT_EXT;
    constexpr VkSpirvResourceTypeFlagsEXT uav_mask = VK_SPIRV_RESOURCE_TYPE_READ_WRITE_IMAGE_BIT_EXT |
                                                     VK_SPIRV_RESOURCE_TYPE_READ_WRITE_STORAGE_BUFFER_BIT_EXT;
    constexpr VkSpirvResourceTypeFlagsEXT cbv_mask = VK_SPIRV_RESOURCE_TYPE_UNIFORM_BUFFER_BIT_EXT;

    layout->internal_state = mkS<VulkanRootSignature>();

    auto vk_root_signature = to_internal(*layout);

    auto params = vk_root_signature->bindings;

    uint32_t push_offset = 0;
    for (unsigned i = 0; i < desc->push_ranges.size(); i++)
    {
        const auto& range = desc->push_ranges[i];
        params.push_back({
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT,
            .descriptorSet = range.space,
            .firstBinding = range.binding,
            .bindingCount = 1,
            .resourceMask = cbv_mask,
            .source = VK_DESCRIPTOR_MAPPING_SOURCE_PUSH_DATA_EXT,
            .sourceData = {.pushDataOffset = push_offset},
        });
        push_offset += range.size;
        assert(push_offset < 128);
    }

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
                            .heapIndexStride = 1, // Byte offset to determine table location
                        },
                    },
            });

            heap_offset += binding.count * descriptor_size;
        }
        push_offset += sizeof(uint32_t);
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
}

bool VulkanContext::set_pipeline_constant(
    CommandList* cmd_list, unsigned param, uint32_t offset, unsigned size, void* data)
{
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

    texture->internal_state = mkS<VulkanTexture>();
    const auto vulkan_texture = static_cast<VulkanTexture*>(texture->internal_state.get());

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
    const VkImageCreateInfo texture_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = image_type,
        .format = convert_format(desc.format),
        .extent = {static_cast<uint32_t>(desc.width), desc.height, is_3D ? desc.depth_or_array_size : 1u},
        .mipLevels = desc.mip_levels,
        .arrayLayers = is_3D ? 1u : desc.depth_or_array_size,
        .samples = static_cast<VkSampleCountFlagBits>(desc.sample_count),
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .sharingMode =
            VK_SHARING_MODE_CONCURRENT, // Concurrent sharing is fine on PC and removes need to transfer ownership
        .initialLayout = layout(desc.initial_layout),
    };

    if (VK_FAILED(vkCreateImage(m_device, &texture_info, nullptr, &vulkan_texture->image)))
    {
        texture->internal_state.reset();
        return false;
    }

    const VmaAllocationCreateInfo alloc_info{VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE};

    if (VK_FAILED(vmaCreateImage(
            m_allocator, &texture_info, &alloc_info, &vulkan_texture->image, &vulkan_texture->allocation, nullptr)))
    {
        texture->internal_state.reset();
        return false;
    }

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

    VkExtent2D extent;
    if (depth_stencil)
    {
        extent = {std::min(static_cast<uint32_t>(depth_stencil->texture->desc.width), swapchain.extent.width),
                  std::min(depth_stencil->texture->desc.height, swapchain.extent.height)};

        auto dsv = to_internal(*depth_stencil->texture);
        const auto& clear_params = depth_stencil->clear_params.dsv_clear_params;
        depth_attachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                            .imageView = VK_NULL_HANDLE, // TODO
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
