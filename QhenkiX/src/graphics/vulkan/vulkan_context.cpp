#include "vulkan_context.h"

#define VOLK_IMPLEMENTATION
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <volk.h>
#include "SDL3/SDL_vulkan.h"
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1004000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0

#include <vk_mem_alloc.h>

#include <Vulkan-Utility/vk_format_utils.h>

#include "vulkan_command_list.h"
#include "vulkan_command_pool.h"
#include "vulkan_descriptor_heap.h"
#include "vulkan_macros.h"
#include "vulkan_pipeline.h"
#include "vulkan_root_signature.h"
#include "vulkan_texture.h"

#include <spirv_glsl.hpp>

#include "qhenki/utility/math_util.h"
#include "qhenki/utility/string_util.h"
#include "src/utility/vulkan_util.h"

constexpr uint32_t PUSH_RESERVED_START_OFFSET = 128u;
constexpr uint32_t MAX_VERTEX_SLOTS = 32u;
constexpr uint32_t MAX_VIEWPORTS_SCISSORS = 16u;

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

VulkanPipeline* to_internal(const GraphicsPipeline& ext)
{
    const auto vulkan_pipeline = static_cast<VulkanPipeline*>(ext.internal_state.get());
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

VulkanCommandList* to_internal(const CommandList& ext)
{
    const auto vulkan_command_buffer = static_cast<VulkanCommandList*>(ext.internal_state.get());
    assert(vulkan_command_buffer);
    return vulkan_command_buffer;
}

void set_debug_name(const VkDevice device, const VkObjectType type, const uint64_t handle, const char* name)
{
    if (vkSetDebugUtilsObjectNameEXT && name)
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

auto get_gpu_address(const VkDevice device, VkBuffer buffer) -> VkDeviceAddress
{
    const VkBufferDeviceAddressInfo addr_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = buffer,
    };
    return vkGetBufferDeviceAddress(device, &addr_info);
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
    std::array<const char*, 8> device_extensions{
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_9_EXTENSION_NAME,
        VK_GOOGLE_HLSL_FUNCTIONALITY_1_EXTENSION_NAME,
        VK_GOOGLE_USER_TYPE_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
        VK_KHR_ROBUSTNESS_2_EXTENSION_NAME,
        VK_EXT_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_EXTENSION_NAME,
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
        .shaderDemoteToHelperInvocation = VK_TRUE,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
        .maintenance4 = VK_TRUE,
    };

    VkPhysicalDeviceVulkan14Features features14{
        .maintenance5 = VK_TRUE,
        .maintenance6 = VK_TRUE,
        .pushDescriptor = VK_TRUE,
    };
    VkPhysicalDeviceDescriptorHeapFeaturesEXT descriptor_heap_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
        .descriptorHeap = VK_TRUE,
    };
    VkPhysicalDeviceRobustness2FeaturesEXT robustness2_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
        .nullDescriptor = VK_TRUE,
    };
    VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT dynamic_rendering_unused_attachments_features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT,
        .dynamicRenderingUnusedAttachments = VK_TRUE,
    };

    auto phys_ret = selector.defer_surface_initialization()
                        .set_minimum_version(major, minor)
                        .add_required_extensions(device_extensions)
                        .set_required_features(features)
                        .set_required_features_11(features11)
                        .set_required_features_12(features12)
                        .set_required_features_13(features13)
                        .set_required_features_14(features14)
                        .add_required_extension_features(descriptor_heap_features)
                        .add_required_extension_features(robustness2_features)
                        .add_required_extension_features(dynamic_rendering_unused_attachments_features)
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

    auto init_queue = [this](const vkb::QueueType type, VulkanQueue& out, const char* name) -> std::string
    {
        auto result = m_device.get_queue_and_index(type);
        if (!result)
        {
            return "Vulkan: Failed to get queue" + result.error().message();
        }
        auto [q, family_index] = result.value();
        out.queue = q;
        out.family_index = family_index;
        set_debug_name(m_device.device, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(q), name);

        const VkSemaphoreTypeCreateInfo semaphore_type_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = 0,
        };
        const VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphore_type_info,
        };
        if (VK_FAILED(vkCreateSemaphore(m_device.device, &semaphore_info, nullptr, &out.semaphore)))
        {
            return "Vulkan: Failed to create queue semaphore";
        }
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "%s Internal Semaphore", name);
        set_debug_name(m_device.device, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(out.semaphore), sem_name);
        return {};
    };

    if (auto err = init_queue(vkb::QueueType::graphics, m_graphics_queue, "Graphics Queue"); !err.empty())
    {
        return err;
    }
    if (auto err = init_queue(vkb::QueueType::compute, m_compute_queue, "Compute Queue"); !err.empty())
    {
        return err;
    }
    if (auto err = init_queue(vkb::QueueType::transfer, m_transfer_queue, "Transfer Queue"); !err.empty())
    {
        return err;
    }

    if (VK_FAILED(vmaCreateAllocator(&allocator_desc, &m_allocator)))
    {
        return "Vulkan: Failed to create VMA allocator";
    }

    vkGetPhysicalDeviceProperties2(m_device.physical_device, &m_capabilities.properties);

    const auto& descriptor_properties = m_capabilities.descriptor_heap_properties;
    const auto required_alignment = std::max(descriptor_properties.bufferDescriptorAlignment,
                                             descriptor_properties.imageDescriptorAlignment);
    m_bloated_resource_descriptor_size = std::max(
        {util::align_up(descriptor_properties.bufferDescriptorSize, required_alignment),
         util::align_up(descriptor_properties.imageDescriptorSize, required_alignment)});

    m_bloated_sampler_descriptor_size = util::align_up(descriptor_properties.samplerDescriptorSize,
                                                       descriptor_properties.samplerDescriptorAlignment);

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
    if (limits.maxViewports < MAX_VIEWPORTS_SCISSORS)
    {
        return "Vulkan: Device does not support required number of viewports";
    }
    if (limits.maxBoundDescriptorSets < MAX_SPACES)
    {
        return "Vulkan: Device does not support required number of spaces";
    }

    const VkSemaphoreTypeCreateInfo internal_semaphore_type{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0,
    };
    const VkSemaphoreCreateInfo internal_semaphore_ci{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &internal_semaphore_type,
    };
    if (VK_FAILED(vkCreateSemaphore(m_device.device, &internal_semaphore_ci, nullptr, &m_internal_semaphore)))
    {
        return "Vulkan: Failed to create internal timeline semaphore";
    }
    set_debug_name(m_device.device,
                   VK_OBJECT_TYPE_SEMAPHORE,
                   reinterpret_cast<uint64_t>(m_internal_semaphore),
                   "Internal Timeline Semaphore");

    constexpr VkSemaphoreCreateInfo image_available_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for (size_t i = 0; i < m_image_available_semaphores.size(); i++)
    {
        if (VK_FAILED(
                vkCreateSemaphore(m_device.device, &image_available_info, nullptr, &m_image_available_semaphores[i])))
        {
            return "Vulkan: Failed to create image-available binary semaphore";
        }
        char avail_name[64];
        snprintf(avail_name, sizeof(avail_name), "Image Available Semaphore [%zu]", i);
        set_debug_name(m_device.device,
                       VK_OBJECT_TYPE_SEMAPHORE,
                       reinterpret_cast<uint64_t>(m_image_available_semaphores[i]),
                       avail_name);

        if (VK_FAILED(
                vkCreateSemaphore(m_device.device, &image_available_info, nullptr, &m_render_finished_semaphores[i])))
        {
            return "Vulkan: Failed to create render-finished binary semaphore";
        }
        char finished_name[64];
        snprintf(finished_name, sizeof(finished_name), "Render Finished Semaphore [%zu]", i);
        set_debug_name(m_device.device,
                       VK_OBJECT_TYPE_SEMAPHORE,
                       reinterpret_cast<uint64_t>(m_render_finished_semaphores[i]),
                       finished_name);
    }

    assert(m_bloated_resource_descriptor_size);
    assert(m_bloated_sampler_descriptor_size);

    return "";
}

bool VulkanContext::is_compatibility() const
{
    return false;
}

bool VulkanContext::create_swapchain(const DisplayWindow& window, const SwapchainDesc& swapchain_desc)
{
    if (!SDL_Vulkan_CreateSurface(window.get_window(), m_instance, nullptr, &m_surface))
    {
        return false;
    }

    return create_swapchain(swapchain_desc);
}

bool VulkanContext::resize_swapchain(Swapchain* swapchain, const unsigned width, const unsigned height)
{
    wait_idle(GRAPHICS);

    m_swapchain.swapchain.destroy_image_views(m_swapchain.image_views);

    SwapchainDesc desc = *swapchain;
    desc.width = width;
    desc.height = height;

    if (!create_swapchain(desc))
    {
        return false;
    }

    *swapchain = desc;
    m_swapchain_index = 0;

    return true;
}

bool VulkanContext::acquire_swapchain_image()
{
    const unsigned sem_index = m_frame_count % m_image_available_semaphores.size();
    if (VK_FAILED(vkAcquireNextImageKHR(m_device.device,
                                        m_swapchain.swapchain.swapchain,
                                        std::numeric_limits<uint64_t>::max(),
                                        m_image_available_semaphores[sem_index],
                                        VK_NULL_HANDLE,
                                        &m_swapchain_index)))
    {
        return false;
    }
    return true;
}

bool VulkanContext::present(const Swapchain& swapchain)
{
    const auto wait_semaphore = m_render_finished_semaphores[m_swapchain_index];
    const VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &wait_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain.swapchain.swapchain,
        .pImageIndices = &m_swapchain_index,
    };
    // TODO: Present on different queue?
    if (VK_FAILED(vkQueuePresentKHR(m_graphics_queue.queue, &present_info)))
    {
        return false;
    }
    ++m_frame_count;
    return true;
}

unsigned VulkanContext::get_frame_slot(const unsigned slot_count) const
{
    return slot_count > 0 ? m_frame_count % slot_count : 0;
}

bool VulkanContext::create_swapchain(const SwapchainDesc& swapchain_desc)
{
    vkb::SwapchainBuilder swapchain_builder{m_device, m_surface};

    const VkPresentModeKHR present_mode = swapchain_desc.tearing ? VK_PRESENT_MODE_IMMEDIATE_KHR
                                                                 : VK_PRESENT_MODE_FIFO_KHR;
    auto swap_ret = swapchain_builder.set_old_swapchain(m_swapchain.swapchain)
                        .set_desired_extent(swapchain_desc.width, swapchain_desc.height)
                        .set_desired_present_mode(present_mode)
                        .set_required_min_image_count(swapchain_desc.buffer_count)
                        .set_desired_format({convert_format(swapchain_desc.format), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                        .build();
    if (!swap_ret)
    {
        m_swapchain.swapchain.swapchain = VK_NULL_HANDLE;
        return false;
    }
    m_swapchain.swapchain = std::move(swap_ret.value());
    set_debug_name(m_device.device,
                   VK_OBJECT_TYPE_SWAPCHAIN_KHR,
                   reinterpret_cast<uint64_t>(m_swapchain.swapchain.swapchain),
                   "Swapchain");

    auto swap_img = m_swapchain.swapchain.get_images();
    if (!swap_img)
    {
        return false;
    }
    m_swapchain.images = std::move(swap_img.value());
    for (size_t i = 0; i < m_swapchain.images.size(); i++)
    {
        char img_name[64];
        snprintf(img_name, sizeof(img_name), "Swapchain Image [%zu]", i);
        set_debug_name(m_device.device,
                       VK_OBJECT_TYPE_IMAGE,
                       reinterpret_cast<uint64_t>(m_swapchain.images[i]),
                       img_name);
    }

    auto swap_img_view = m_swapchain.swapchain.get_image_views();
    if (!swap_img_view)
    {
        return false;
    }
    m_swapchain.image_views = std::move(swap_img_view.value());
    for (size_t i = 0; i < m_swapchain.image_views.size(); i++)
    {
        char view_name[64];
        snprintf(view_name, sizeof(view_name), "Swapchain Image View [%zu]", i);
        set_debug_name(m_device.device,
                       VK_OBJECT_TYPE_IMAGE_VIEW,
                       reinterpret_cast<uint64_t>(m_swapchain.image_views[i]),
                       view_name);
    }

    // Transition swapchain images from UNDEFINED to PRESENT_SRC_KHR
    {
        auto& internal_pool = acquire_command_pool(GRAPHICS);
        if (internal_pool.command_pool == VK_NULL_HANDLE)
        {
            return false;
        }

        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        const auto cmd = internal_pool.create_command_buffer(alloc_info);
        if (cmd == VK_NULL_HANDLE)
        {
            return false;
        }

        constexpr VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(cmd, &begin_info);

        const auto image_count = static_cast<uint32_t>(m_swapchain.images.size());
        VkImageMemoryBarrier2 barriers[4]{};
        assert(image_count <= 4);
        for (uint32_t i = 0; i < image_count; i++)
        {
            barriers[i] = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_NONE,
                .dstAccessMask = VK_ACCESS_2_NONE,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = m_swapchain.images[i],
                .subresourceRange =
                    {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    },
            };
        }

        const VkDependencyInfo dep_info{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = image_count,
            .pImageMemoryBarriers = barriers,
        };
        vkCmdPipelineBarrier2(cmd, &dep_info);
        vkEndCommandBuffer(cmd);

        ++m_internal_semaphore_value;
        const VkCommandBufferSubmitInfo cmd_submit_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = cmd,
        };
        const VkSemaphoreSubmitInfo signal_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_internal_semaphore,
            .value = m_internal_semaphore_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        };
        const VkSubmitInfo2 submit{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &cmd_submit_info,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signal_info,
        };
        if (VK_FAILED(vkQueueSubmit2(m_graphics_queue.queue, 1, &submit, VK_NULL_HANDLE)))
        {
            return false;
        }

        const VkSemaphoreWaitInfo wait_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores = &m_internal_semaphore,
            .pValues = &m_internal_semaphore_value,
        };
        vkWaitSemaphores(m_device.device, &wait_info, std::numeric_limits<uint64_t>::max());

        internal_pool.reset();
    }

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
                                    const Shader vertex_shader,
                                    const Shader pixel_shader,
                                    PipelineLayout* in_layout,
                                    const char* debug_name)
{
    if (desc.num_render_targets > MAX_RENDER_TARGETS)
    {
        return false;
    }

    const spirv_cross::CompilerGLSL vs_reflect(static_cast<uint32_t*>(vertex_shader.data), vertex_shader.size / 4u);
    spirv_cross::ShaderResources resources = vs_reflect.get_shader_resources();

    const auto vs_entry_name = vs_reflect.get_entry_points_and_stages()[0].name;

    thread_local memory::Arena arena(util::MEGABYTE);
    arena.reset();

    const auto input_count = resources.stage_inputs.size();
    VkVertexInputAttributeDescription* input_attributes = nullptr;
    if (input_count > 0)
    {
        input_attributes = arena.alloc_array<VkVertexInputAttributeDescription>(input_count);
    }

    const uint32_t binding_count = desc.increment_slot ? static_cast<uint32_t>(input_count) : input_count > 0 ? 1u : 0u;
    VkVertexInputBindingDescription* binding_descs = nullptr;
    if (binding_count > 0)
    {
        binding_descs = arena.alloc_array<VkVertexInputBindingDescription>(binding_count);
    }

    uint32_t offset = 0;
    for (uint32_t i = 0; i < input_count; i++)
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

    const spirv_cross::CompilerGLSL ps_reflect(static_cast<uint32_t*>(pixel_shader.data), pixel_shader.size / 4u);
    const auto ps_entry_name = ps_reflect.get_entry_points_and_stages()[0].name;

    const auto vk_root_signature = to_internal(*in_layout);

    VkShaderModuleCreateInfo vs_module_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = &vk_root_signature->layout,
        .codeSize = vertex_shader.size,
        .pCode = static_cast<const uint32_t*>(vertex_shader.data),
    };

    VkShaderModuleCreateInfo ps_module_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = &vk_root_signature->layout,
        .codeSize = pixel_shader.size,
        .pCode = static_cast<const uint32_t*>(pixel_shader.data),
    };

    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages;
    shader_stages[0] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &vs_module_info,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .pName = vs_entry_name.c_str(),
    };
    shader_stages[1] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = &ps_module_info,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pName = ps_entry_name.c_str(),
    };

    VkPipelineVertexInputStateCreateInfo vertex_input_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = binding_count,
        .pVertexBindingDescriptions = binding_descs,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(input_count),
        .pVertexAttributeDescriptions = input_attributes,
    };

    // We require multi-viewport device feature
    VkPipelineViewportStateCreateInfo viewport_state_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    };

    std::array dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
        VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,
        VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
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
        const auto polygon_mode = static_cast<VkPolygonMode>(1 - (r.fill_mode - 2));

        VkCullModeFlags cull_mode = static_cast<VkCullModeFlags>(r.cull_mode - 1);

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
        alpha_to_coverage = desc.blend_desc->alpha_to_coverage_enable;
    }

    VkPipelineMultisampleStateCreateInfo multisample_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = static_cast<VkSampleCountFlagBits>(desc.sample_count),
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = alpha_to_coverage,
        .alphaToOneEnable = VK_FALSE,
    };

    auto map_stencil_op = [](const StencilOp op)
    {
        return static_cast<VkStencilOp>(op - 1);
    };

    auto map_compare_func = [](const ComparisonFunc func)
    {
        return static_cast<VkCompareOp>(func - 1);
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    const bool has_depth_stencil_desc = desc.depth_stencil_state.has_value();
    VkFormat depth_stencil_format = VK_FORMAT_UNDEFINED;
    if (desc.dsv_format != Format::UNKNOWN)
    {
        depth_stencil_format = convert_format(desc.dsv_format);
    }

    const bool has_depth_attachment = has_depth_stencil_desc && depth_stencil_format != VK_FORMAT_UNDEFINED &&
                                      is_depth_stencil_format(depth_stencil_format);

    if (has_depth_attachment)
    {
        const auto& ds = *desc.depth_stencil_state;

        const auto make_stencil_state = [&](const DepthStencilOpDesc& op)
        {
            VkStencilOpState state;
            state.failOp = map_stencil_op(op.fail_op);
            state.passOp = map_stencil_op(op.pass_op);
            state.depthFailOp = map_stencil_op(op.depth_fail_op);
            state.compareOp = map_compare_func(op.func);
            state.compareMask = ds.stencil_read_mask;
            state.writeMask = ds.stencil_write_mask;
            state.reference = 0;
            return state;
        };

        depth_stencil_info = {
            .depthTestEnable = ds.depth_enable ? VK_TRUE : VK_FALSE,
            .depthWriteEnable = ds.depth_write_enable,
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

    auto map_blend_op = [](const BlendOp op)
    {
        return static_cast<VkBlendOp>(op - 1);
    };

    std::array<VkPipelineColorBlendAttachmentState, MAX_RENDER_TARGETS> color_attachments{};
    for (unsigned i = 0; i < desc.num_render_targets; i++)
    {
        VkPipelineColorBlendAttachmentState attachment{};
        if (desc.blend_desc.has_value())
        {
            const auto& bd = *desc.blend_desc;
            const auto& rt0 = bd.render_target[0];
            const auto& rt = bd.independent_blend_enable ? bd.render_target[i] : rt0;
            attachment.blendEnable = rt0.logic_op_enable ? VK_FALSE : (rt.blend_enable ? VK_TRUE : VK_FALSE);
            attachment.srcColorBlendFactor = blend_factor(rt.src_blend);
            attachment.dstColorBlendFactor = blend_factor(rt.dst_blend);
            attachment.colorBlendOp = map_blend_op(rt.blend_op);
            attachment.srcAlphaBlendFactor = blend_factor(rt.src_blend_alpha);
            attachment.dstAlphaBlendFactor = blend_factor(rt.dst_blend_alpha);
            attachment.alphaBlendOp = map_blend_op(rt.blend_op_alpha);
            // Directly compatible since DX and VK use same bitmask
            attachment.colorWriteMask = rt.render_target_write_mask;
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

    VkBool32 logic_op_enable = VK_FALSE;
    VkLogicOp logic_op = VK_LOGIC_OP_COPY;
    if (desc.blend_desc.has_value() && desc.num_render_targets > 0)
    {
        const auto& rt0 = desc.blend_desc->render_target[0];
        logic_op_enable = rt0.logic_op_enable ? VK_TRUE : VK_FALSE;
        logic_op = vk_logic_op(rt0.logic_op);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = logic_op_enable,
        .logicOp = logic_op,
        .attachmentCount = desc.num_render_targets,
        .pAttachments = color_attachments.data(),
    };

    std::array<VkFormat, MAX_RENDER_TARGETS> color_formats{};
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
        .stencilAttachmentFormat = has_depth_attachment &&
                                           (get_image_aspect_mask(depth_stencil_format) & VK_IMAGE_ASPECT_STENCIL_BIT)
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

    pipeline->internal_state = mkS<VulkanPipeline>(vk_pipeline, primitive_topology, vk_root_signature);

    set_debug_name(m_device.device, VK_OBJECT_TYPE_PIPELINE, reinterpret_cast<uint64_t>(vk_pipeline), debug_name);

    return true;
}

bool VulkanContext::bind_pipeline(CommandList* cmd_list, const GraphicsPipeline& pipeline)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    const auto vk_pipeline = to_internal(pipeline);

    vkCmdSetPrimitiveTopology(vk_cmd_list->cmd_buf, vk_pipeline->topology);
    vkCmdBindPipeline(vk_cmd_list->cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->pipeline);

    assert(vk_pipeline->root_signature);
    vk_cmd_list->root_signature = vk_pipeline->root_signature;

    return true;
}

bool VulkanContext::create_pipeline_layout(PipelineLayoutDesc* desc, PipelineLayout* layout)
{
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

    auto& params = vk_root_signature->bindings;

    uint32_t push_offset = 0;
    for (unsigned i = 0; i < desc->push_ranges.size(); i++)
    {
        const auto& range = desc->push_ranges[i];
        if (range.size % 4u != 0)
        {
            params.clear();
            return false;
        }
        push_offset += util::ceil_div(range.size, 4u);
        assert(push_offset < PUSH_RESERVED_START_OFFSET);
    }

    vk_root_signature->push_range_count = static_cast<uint32_t>(desc->push_ranges.size());

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
            const uint32_t descriptor_size = binding.type == LayoutBinding::SAMPLER
                                               ? m_bloated_sampler_descriptor_size
                                               : m_bloated_resource_descriptor_size;

            switch (binding.type)
            {
            case LayoutBinding::SRV:
                mask = srv_mask;
                break;
            case LayoutBinding::UAV:
                mask = uav_mask;
                break;
            case LayoutBinding::CBV:
                mask = cbv_mask;
                break;
            case LayoutBinding::SAMPLER:
                mask = sampler_mask;
                break;
            }

            assert(mask);
            if (j != 0)
            {
                const auto& prev_binding = space[j - 1];
                const bool prev_sampler = prev_binding.type == LayoutBinding::SAMPLER;
                const bool cur_sampler = binding.type == LayoutBinding::SAMPLER;
                if (prev_sampler != cur_sampler)
                {
                    params.clear();
                    return false;
                }
            }

            const uint32_t heap_array_stride = binding.count > 1 ? descriptor_size : 0u;

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
                            .heapIndexStride = 1,
                            .heapArrayStride = heap_array_stride,
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

bool VulkanContext::set_pipeline_constant(CommandList* cmd_list,
                                          const PipelineLayout& expected_layout,
                                          unsigned param,
                                          const uint32_t offset,
                                          const unsigned size,
                                          void* data)
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
    const auto vk_layout = to_internal(expected_layout);

    if (vk_cmd_list->root_signature != vk_layout)
    {
        return false;
    }

    const VkPushDataInfoEXT push_data_info{.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
                                           .offset = 0 + offset, // TODO: Use param to calculate offset
                                           .data = {.address = data, .size = size}};
    assert(push_data_info.offset % 4u == 0);
    vkCmdPushDataEXT(vk_cmd_list->cmd_buf, &push_data_info);

    return true;
}

bool VulkanContext::create_descriptor_heap(const DescriptorHeapDesc& desc, DescriptorHeap* heap, const char* debug_name)
{
    heap->internal_state = mkS<VulkanDescriptorHeap>();
    const auto vk_heap = to_internal(*heap);

    const auto& properties = m_capabilities.descriptor_heap_properties;
    VulkanDescriptorHeapInitInfo init_info{
        .allocator = m_allocator,
    };
    if (desc.type == DescriptorHeapDesc::Type::CBV_SRV_UAV)
    {
        init_info.heap_info = {
            .reserved_size = properties.minResourceHeapReservedRange,
            .max_size = properties.maxResourceHeapSize,
            .heap_alignment = properties.resourceHeapAlignment,
            .descriptor_size = m_bloated_resource_descriptor_size,
        };
    }
    else
    {
        init_info.heap_info = {
            .reserved_size = properties.minSamplerHeapReservedRange,
            .max_size = properties.maxSamplerHeapSize,
            .heap_alignment = properties.samplerHeapAlignment,
            .descriptor_size = m_bloated_sampler_descriptor_size,
        };
    }

    if (!vk_heap->create(desc, init_info))
    {
        heap->internal_state.reset();
        return false;
    }

    heap->desc = desc;

    set_debug_name(m_device.device,
                   VK_OBJECT_TYPE_BUFFER,
                   reinterpret_cast<uint64_t>(vk_heap->get_buffer()),
                   debug_name);

    return true;
}

void VulkanContext::set_descriptor_heap(CommandList* cmd_list, const DescriptorHeap& heap)
{
    const auto vk_heap = to_internal(heap);

    const VkDeviceAddressRangeEXT heap_range{
        .address = get_gpu_address(m_device.device, vk_heap->get_buffer()),
        .size = vk_heap->get_total_size(),
    };

    const VkBindHeapInfoEXT bind_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange = heap_range,
        .reservedRangeOffset = 0,
        .reservedRangeSize = m_capabilities.descriptor_heap_properties.minResourceHeapReservedRange,
    };

    const auto vk_cmd_list = to_internal(*cmd_list);

    vkCmdBindResourceHeapEXT(vk_cmd_list->cmd_buf, &bind_heap_info);
}

void VulkanContext::set_descriptor_heap(CommandList* cmd_list,
                                        const DescriptorHeap& heap,
                                        const DescriptorHeap& sampler_heap)
{
    const auto vk_heap = to_internal(heap);
    const auto vk_sampler_heap = to_internal(sampler_heap);

    const VkDeviceAddressRangeEXT heap_range{
        .address = get_gpu_address(m_device.device, vk_heap->get_buffer()),
        .size = vk_heap->get_total_size(),
    };

    const VkBindHeapInfoEXT bind_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange = heap_range,
        .reservedRangeOffset = 0,
        .reservedRangeSize = m_capabilities.descriptor_heap_properties.minResourceHeapReservedRange,
    };

    const VkDeviceAddressRangeEXT sampler_heap_range{
        .address = get_gpu_address(m_device.device, vk_sampler_heap->get_buffer()),
        .size = vk_sampler_heap->get_total_size(),
    };

    const VkBindHeapInfoEXT sampler_bind_heap_info{
        .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
        .heapRange = sampler_heap_range,
        .reservedRangeOffset = 0,
        .reservedRangeSize = m_capabilities.descriptor_heap_properties.minSamplerHeapReservedRange,
    };

    const auto vk_cmd_list = to_internal(*cmd_list);

    vkCmdBindResourceHeapEXT(vk_cmd_list->cmd_buf, &bind_heap_info);
    vkCmdBindSamplerHeapEXT(vk_cmd_list->cmd_buf, &sampler_bind_heap_info);
}

bool VulkanContext::set_descriptor_table(CommandList* cmd_list,
                                         const PipelineLayout& expected_layout,
                                         const unsigned index,
                                         const Descriptor& gpu_descriptor)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    const auto vk_layout = to_internal(expected_layout);

    if (vk_cmd_list->root_signature != vk_layout)
    {
        return false;
    }

    if (index < vk_cmd_list->root_signature->push_range_count)
    {
        return false;
    }
    const unsigned table_index = index - vk_cmd_list->root_signature->push_range_count;
    if (table_index >= vk_cmd_list->root_signature->bindings.size())
    {
        return false;
    }

    const auto reserved = get_reserved_range(gpu_descriptor.heap->desc.type);
    const auto stride = gpu_descriptor.heap->desc.type == DescriptorHeapDesc::Type::SAMPLER
                          ? m_bloated_sampler_descriptor_size
                          : m_bloated_resource_descriptor_size;
    const auto byte_offset = reserved + gpu_descriptor.offset * stride;
    const uint32_t absolute_offset = static_cast<uint32_t>(byte_offset);

    const VkPushDataInfoEXT push_data_info{.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
                                           .offset = static_cast<uint32_t>(PUSH_RESERVED_START_OFFSET +
                                                                           table_index * sizeof(uint32_t)),
                                           .data = {
                                               .address = &absolute_offset,
                                               .size = sizeof(uint32_t),
                                           }};
    vkCmdPushDataEXT(vk_cmd_list->cmd_buf, &push_data_info);

    return true;
}

bool VulkanContext::copy_descriptors(const size_t num_descriptors, const Descriptor& src, const Descriptor& dst)
{
    if (src.heap->desc.type != dst.heap->desc.type)
    {
        return false;
    }

    if (src.heap->desc.visibility != DescriptorHeapDesc::Visibility::CPU)
    {
        return false;
    }

    m_descriptor_copier.add_pending_descriptor_copy(num_descriptors, src, dst);
    return true;
}

bool VulkanContext::create_buffer(const BufferDesc& desc, const void* data, Buffer* buffer, const char* debug_name)
{
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
    buffer_info.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
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
    return create_descriptor_buffer(buffer, heap, descriptor, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
}

bool VulkanContext::create_descriptor_shader_view(const Buffer& buffer, DescriptorHeap* heap, Descriptor* descriptor)
{
    return create_descriptor_buffer(buffer, heap, descriptor, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
}

void VulkanContext::copy_buffer(
    CommandList* cmd_list, const Buffer& src, uint64_t src_offset, Buffer* dst, uint64_t dst_offset, uint64_t bytes)
{
    assert(src_offset + bytes <= src.desc.size);
    assert(dst_offset + bytes <= dst->desc.size);

    const auto vk_cmd_list = to_internal(*cmd_list);
    const auto vk_src = to_internal(src);
    const auto vk_dst = to_internal(*dst);

    const VkBufferCopy2 region{
        .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
        .srcOffset = src_offset,
        .dstOffset = dst_offset,
        .size = bytes,
    };
    const VkCopyBufferInfo2 copy_info{
        .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
        .srcBuffer = vk_src->buffer,
        .dstBuffer = vk_dst->buffer,
        .regionCount = 1,
        .pRegions = &region,
    };
    vkCmdCopyBuffer2(vk_cmd_list->cmd_buf, &copy_info);
}

bool VulkanContext::create_texture(const TextureDesc& desc, Texture* texture, const char* debug_name)
{
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
        .extent = {desc.width, desc.height, is_3D ? desc.depth_or_array_size : 1u},
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
    // Transitions are deferred
    vulkan_texture->initial_layout = layout(desc.initial_layout);
    texture->desc = desc;

    set_debug_name(m_device.device,
                   VK_OBJECT_TYPE_IMAGE,
                   reinterpret_cast<uint64_t>(vulkan_texture->image),
                   debug_name);

    if (!m_texture_queue.enqueue(texture))
    {
        return false;
    }

    return true;
}

bool VulkanContext::create_descriptor_shader_view(const Texture& texture, DescriptorHeap* heap, Descriptor* descriptor)
{
    return create_descriptor_texture(texture, heap, descriptor, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
}

namespace
{
bool compute_vk_copy_pitch(
    const VkFormat format, const uint32_t width, const uint32_t height, size_t* slice_pitch, const uint32_t depth)
{
    if (vkuFormatIsUndefined(format) || vkuFormatIsMultiplane(format))
    {
        return false;
    }

    const uint32_t bytes_per_block = vkuFormatTexelBlockSize(format);
    const VkExtent3D block_extent = vkuFormatTexelBlockExtent(format);
    if (bytes_per_block == 0 || block_extent.width == 0 || block_extent.height == 0)
    {
        return false;
    }

    const uint32_t blocks_x = qhenki::util::ceil_div(width, block_extent.width);
    const uint32_t blocks_y = qhenki::util::ceil_div(height, block_extent.height);

    const size_t row_pitch = blocks_x * bytes_per_block;
    *slice_pitch = row_pitch * blocks_y * depth;

    return true;
}
} // namespace

uint64_t VulkanContext::get_required_staging_size(const Texture& texture)
{
    const auto& desc = texture.desc;

    const bool is_3D = desc.dimension == TextureDimension::TEXTURE_3D;
    const uint32_t num_subresources = is_3D ? desc.mip_levels : desc.mip_levels * desc.depth_or_array_size;
    const auto vk_format = convert_format(desc.format);
    const size_t staging_alignment = get_staging_alignment(texture);

    size_t total_size = 0;
    for (uint32_t subresource = 0; subresource < num_subresources; subresource++)
    {
        uint32_t mip;

        if (is_3D)
        {
            mip = subresource;
        }
        else
        {
            mip = subresource % desc.mip_levels;
        }

        const uint32_t mip_width = std::max(1u, desc.width >> mip);
        const uint32_t mip_height = std::max(1u, desc.height >> mip);
        const uint32_t mip_depth = is_3D ? std::max(1u, static_cast<uint32_t>(desc.depth_or_array_size) >> mip) : 1u;

        size_t slice_pitch = 0;
        if (!compute_vk_copy_pitch(vk_format, mip_width, mip_height, &slice_pitch, mip_depth))
        {
            return false;
        }

        total_size = util::align_up(total_size, staging_alignment);
        total_size += slice_pitch;
    }
    return total_size;
}

size_t VulkanContext::get_staging_alignment(const Texture& texture)
{
    constexpr size_t min_copy_offset_alignment = 4;
    const auto format = convert_format(texture.desc.format);

    if (vkuFormatIsUndefined(format) || vkuFormatIsMultiplane(format))
    {
        return min_copy_offset_alignment;
    }

    const size_t bytes_per_block = vkuFormatTexelBlockSize(format);
    if (bytes_per_block == 0)
    {
        return min_copy_offset_alignment;
    }

    size_t alignment = min_copy_offset_alignment;
    while (alignment % bytes_per_block != 0)
    {
        alignment += min_copy_offset_alignment;
    }

    return alignment;
}

bool VulkanContext::copy_to_texture(CommandList* cmd_list,
                                    const void* data,
                                    const BufferRange staging,
                                    Texture* texture)
{
    const auto tex = to_internal(*texture);
    const TextureDesc& desc = texture->desc;

    const bool is_3D = desc.dimension == TextureDimension::TEXTURE_3D;
    const uint32_t num_subresources = is_3D ? desc.mip_levels : desc.mip_levels * desc.depth_or_array_size;

    auto& arena = acquire_arena(m_frame_count);
    const auto regions = arena.alloc_array<VkBufferImageCopy>(num_subresources);

    const auto vk_format = convert_format(desc.format);
    const size_t staging_alignment = get_staging_alignment(*texture);

    static_assert(std::is_same_v<VkDeviceSize, size_t>);
    size_t total_size = 0;
    for (uint32_t subresource = 0; subresource < num_subresources; subresource++)
    {
        uint32_t mip;
        uint32_t layer;

        if (is_3D)
        {
            mip = subresource;
            layer = 0;
        }
        else
        {
            mip = subresource % desc.mip_levels;
            layer = subresource / desc.mip_levels;
        }

        const uint32_t mip_width = std::max(1u, desc.width >> mip);
        const uint32_t mip_height = std::max(1u, desc.height >> mip);
        const uint32_t mip_depth = is_3D ? std::max(1u, static_cast<uint32_t>(desc.depth_or_array_size) >> mip) : 1u;

        size_t slice_pitch = 0;
        if (!compute_vk_copy_pitch(vk_format, mip_width, mip_height, &slice_pitch, mip_depth))
        {
            return false;
        }

        total_size = util::align_up(total_size, staging_alignment);
        regions[subresource] = {
            .bufferOffset = total_size + staging.offset,
            .bufferRowLength = 0,   // Tightly packed
            .bufferImageHeight = 0, // Tightly packed
            .imageSubresource =
                {
                    .aspectMask = get_image_aspect_mask(vk_format),
                    .mipLevel = mip,
                    .baseArrayLayer = is_3D ? 0 : layer,
                    .layerCount = 1,
                },
            .imageOffset = {0, 0, 0},
            .imageExtent = {mip_width, mip_height, mip_depth},
        };

        total_size += slice_pitch;
    }

    const auto upload = static_cast<uint8_t*>(map_buffer(*staging.buffer));
    size_t data_offset = 0;

    // Second pass: pack data into the staging buffer
    for (uint32_t subresource = 0; subresource < num_subresources; ++subresource)
    {
        const uint32_t mip = is_3D ? subresource : subresource % desc.mip_levels;

        const uint32_t mip_width = std::max(1u, desc.width >> mip);
        const uint32_t mip_height = std::max(1u, desc.height >> mip);
        const uint32_t mip_depth = is_3D ? std::max(1u, static_cast<uint32_t>(desc.depth_or_array_size) >> mip) : 1u;

        size_t src_slice_pitch = 0;
        if (!compute_vk_copy_pitch(vk_format, mip_width, mip_height, &src_slice_pitch, mip_depth))
        {
            return false;
        }

        memcpy(upload + regions[subresource].bufferOffset,
               static_cast<const uint8_t*>(data) + data_offset,
               src_slice_pitch);

        data_offset += src_slice_pitch;
    }

    unmap_buffer(*staging.buffer);

    // Texture should have already been transitioned to TRANSFER_DST by prepended command list in submit_command_lists

    const auto vk_cmd_list = to_internal(*cmd_list);
    const auto vk_buffer = to_internal(*staging.buffer);
    vkCmdCopyBufferToImage(vk_cmd_list->cmd_buf,
                           vk_buffer->buffer,
                           tex->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           num_subresources,
                           regions);

    return true;
}

bool VulkanContext::create_descriptor(const SamplerDesc& desc, DescriptorHeap* heap, Descriptor* descriptor)
{
    const auto vk_heap = to_internal(*heap);

    const VkSamplerCreateInfo sampler_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        // Directly compatible
        .magFilter = static_cast<VkFilter>(desc.mag_filter),
        .minFilter = static_cast<VkFilter>(desc.min_filter),
        .mipmapMode = static_cast<VkSamplerMipmapMode>(desc.mip_filter),
        .addressModeU = texture_address_mode(desc.address_mode_u),
        .addressModeV = texture_address_mode(desc.address_mode_v),
        .addressModeW = texture_address_mode(desc.address_mode_w),
        .mipLodBias = desc.mip_lod_bias,
        .anisotropyEnable = desc.max_anisotropy > 0,
        .maxAnisotropy = static_cast<float>(desc.max_anisotropy),
        .compareEnable = desc.comparison_enable,
        .compareOp = static_cast<VkCompareOp>(desc.comparison_func - 1),
        .minLod = desc.min_lod,
        .maxLod = desc.max_lod,
    };

    const auto address = vk_heap->get_cpu_pointer(descriptor->offset * m_bloated_sampler_descriptor_size);
    const VkHostAddressRangeEXT range{
        .address = address,
        .size = m_capabilities.descriptor_heap_properties.samplerDescriptorSize,
    };

    descriptor->heap = heap;

    return VK_SUCCEEDED(vkWriteSamplerDescriptorsEXT(m_device.device, 1, &sampler_info, &range));
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
        if (buffers[i])
        {
            const auto vk_buffer = to_internal(*buffers[i]);
            vk_buffers[i] = vk_buffer->buffer;
        }
        else
        {
            vk_buffers[i] = VK_NULL_HANDLE;
        }
    }

    vkCmdBindVertexBuffers2(vk_cmd_list->cmd_buf, start_slot, buffer_count, vk_buffers.data(), offsets, sizes, strides);
    return true;
}

void VulkanContext::bind_index_buffer(CommandList* cmd_list,
                                      const Buffer& buffer,
                                      const IndexType format,
                                      const uint64_t offset)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    const auto vk_buffer = to_internal(buffer);

    vkCmdBindIndexBuffer2(vk_cmd_list->cmd_buf,
                          vk_buffer->buffer,
                          offset,
                          VK_WHOLE_SIZE,
                          format == IndexType::UINT32 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16);
}

bool VulkanContext::create_command_pool(CommandPool* command_pool, const QueueType queue, const char* debug_name)
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

    set_debug_name(m_device.device, VK_OBJECT_TYPE_COMMAND_POOL, reinterpret_cast<uint64_t>(pool), debug_name);

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
    cmd_list->internal_state = mkS<VulkanCommandList>(cmd_buffer);

    const auto vk_cmd_list = to_internal(*cmd_list);

    if (debug_name)
    {
        strncpy(vk_cmd_list->debug_name.data(), debug_name, vk_cmd_list->debug_name.size() - 1);
        vk_cmd_list->debug_name.back() = '\0';
    }

    set_debug_name(m_device.device, VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(cmd_buffer), debug_name);

    return true;
}

bool VulkanContext::reset_command_list(CommandList* cmd_list, const CommandPool& command_pool)
{
    auto vk_cmd_list = to_internal(*cmd_list);

    if (!create_command_list(cmd_list, command_pool, vk_cmd_list->debug_name.data()))
    {
        return false;
    }

    vk_cmd_list = to_internal(*cmd_list);

    constexpr VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    if (VK_FAILED(vkBeginCommandBuffer(vk_cmd_list->cmd_buf, &begin_info)))
    {
        return false;
    }

    return true;
}

bool VulkanContext::close_command_list(CommandList* cmd_list)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    return VK_SUCCEEDED(vkEndCommandBuffer(vk_cmd_list->cmd_buf));
}

void VulkanContext::end_render_pass(CommandList* cmd_list)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    vkCmdEndRendering(vk_cmd_list->cmd_buf);
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
                  const RenderTarget* targets,
                  const RenderTarget* depth_stencil,
                  RenderTargetState* state)
{
    for (unsigned i = 0; i < count; i++)
    {
        const auto tex = targets[i].texture;
        assert(tex->desc.usage & TextureDesc::RENDER_TARGET);
        const auto vk_texture = to_internal(*tex);
        const VkImageViewCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vk_texture->image,
            .viewType = view_type_from_desc(tex->desc),
            .format = convert_format(tex->desc.format),
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
        const auto tex = depth_stencil->texture;
        const auto vk_texture = to_internal(*tex);
        assert(tex->desc.usage & TextureDesc::DEPTH_STENCIL);
        const auto vk_format = convert_format(tex->desc.format);

        const VkImageViewCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vk_texture->image,
            .viewType = view_type_from_desc(tex->desc),
            .format = vk_format,
            .subresourceRange = // TODO: Specific mips
            {
                .aspectMask = get_image_aspect_mask(vk_format),
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
                                      const RenderTarget* depth_stencil)
{
    VkRenderingAttachmentInfo color_attachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_swapchain.image_views[m_swapchain_index],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
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
    if (!create_views(m_device.device, 0, nullptr, depth_stencil, &rt_state))
    {
        return false;
    }

    VkExtent2D extent;
    if (depth_stencil)
    {
        assert(depth_stencil->clear_type & RenderTarget::DEPTH || depth_stencil->clear_type & RenderTarget::STENCIL);
        extent = {std::min(depth_stencil->texture->desc.width, swapchain.extent.width),
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

    const bool has_stencil = depth_stencil &&
                             (convert_format(depth_stencil->texture->desc.format) == VK_FORMAT_D24_UNORM_S8_UINT ||
                              convert_format(depth_stencil->texture->desc.format) == VK_FORMAT_D32_SFLOAT_S8_UINT);

    const VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = render_area,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = depth_stencil ? &depth_attachment : nullptr,
        .pStencilAttachment = has_stencil ? &depth_attachment : nullptr,
    };

    const auto vk_cmd_list = to_internal(*cmd_list);
    vkCmdBeginRendering(vk_cmd_list->cmd_buf, &rendering_info);

    return true;
}

bool VulkanContext::start_render_pass(CommandList* cmd_list,
                                      const unsigned rt_count,
                                      const RenderTarget* rts,
                                      const RenderTarget* depth_stencil)
{
    auto& rt_state = get_render_target_state(&rts, depth_stencil);
    if (!create_views(m_device.device, rt_count, rts, depth_stencil, &rt_state))
    {
        return false;
    }

    VkExtent2D extent{std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()};
    std::array<VkRenderingAttachmentInfo, MAX_RENDER_TARGETS> color_attachments;
    for (unsigned i = 0; i < rt_count; i++)
    {
        const auto& clear_params = rts[i].clear_params;
        color_attachments[i] = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = rt_state.color_render_targets[i],
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue =
                {
                    .color = {clear_params.clear_color_value[0],
                              clear_params.clear_color_value[1],
                              clear_params.clear_color_value[2],
                              clear_params.clear_color_value[3]},
                },
        };
        extent = {
            std::min(extent.width, rts[i].texture->desc.width),
            std::min(extent.height, rts[i].texture->desc.height),
        };
    }

    VkRenderingAttachmentInfo depth_attachment;
    if (depth_stencil)
    {
        assert(depth_stencil->clear_type & RenderTarget::DEPTH || depth_stencil->clear_type & RenderTarget::STENCIL);
        extent = {std::min(extent.width, static_cast<uint32_t>(depth_stencil->texture->desc.width)),
                  std::min(extent.height, static_cast<uint32_t>(depth_stencil->texture->desc.height))};
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

    const VkRect2D render_area{
        .offset = {0, 0},
        .extent = extent,
    };

    const bool has_stencil = depth_stencil &&
                             (convert_format(depth_stencil->texture->desc.format) == VK_FORMAT_D24_UNORM_S8_UINT ||
                              convert_format(depth_stencil->texture->desc.format) == VK_FORMAT_D32_SFLOAT_S8_UINT);

    const VkRenderingInfo rendering_info{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = render_area,
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = rt_count,
        .pColorAttachments = color_attachments.data(),
        .pDepthAttachment = depth_stencil ? &depth_attachment : nullptr,
        .pStencilAttachment = has_stencil ? &depth_attachment : nullptr,
    };

    const auto vk_cmd_list = to_internal(*cmd_list);
    vkCmdBeginRendering(vk_cmd_list->cmd_buf, &rendering_info);

    return true;
}

void VulkanContext::set_viewports(CommandList* list, const unsigned count, const Viewport* viewport)
{
    std::array<VkViewport, MAX_VIEWPORTS_SCISSORS> vk_viewports;
    for (unsigned i = 0; i < count; i++)
    {
        vk_viewports[i] = {
            .x = viewport[i].top_left_x,
            .y = viewport[i].top_left_y,
            .width = viewport[i].width,
            .height = viewport[i].height,
            .minDepth = viewport[i].min_depth,
            .maxDepth = viewport[i].max_depth,
        };
    }
    const auto vk_cmd_list = to_internal(*list);
    vkCmdSetViewportWithCount(vk_cmd_list->cmd_buf, count, vk_viewports.data());
}

void VulkanContext::set_scissor_rects(CommandList* list, const unsigned count, const Rect* scissor_rect)
{
    std::array<VkRect2D, MAX_VIEWPORTS_SCISSORS> vk_scissors;
    for (unsigned i = 0; i < count; i++)
    {
        vk_scissors[i] = {
            .offset = {scissor_rect[i].left, scissor_rect[i].top},
            .extent = {scissor_rect[i].width, scissor_rect[i].height},
        };
    }
    const auto vk_cmd_list = to_internal(*list);
    vkCmdSetScissorWithCount(vk_cmd_list->cmd_buf, count, vk_scissors.data());
}

void VulkanContext::draw(CommandList* cmd_list, const uint32_t vertex_count, const uint32_t start_vertex_offset)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    vkCmdDraw(vk_cmd_list->cmd_buf, vertex_count, 1, start_vertex_offset, 0);
}

void VulkanContext::draw_indexed(CommandList* cmd_list,
                                 const uint32_t index_count,
                                 const uint32_t instance_count,
                                 const uint32_t start_index_offset,
                                 const int32_t base_vertex_offset,
                                 const uint32_t instance_offset)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    vkCmdDrawIndexed(
        vk_cmd_list->cmd_buf, index_count, instance_count, start_index_offset, base_vertex_offset, instance_offset);
}

bool VulkanContext::submit_command_lists(const SubmitInfo& submit_info, const QueueType queue)
{
    std::scoped_lock lock(m_submit_mutex);
    // Internally ordered within the same queue so treat this as an error
    for (unsigned i = 0; i < submit_info.wait_fence_count; i++)
    {
        if (submit_info.wait_queues[i] == queue)
        {
            return false;
        }
    }

    auto& arena = acquire_arena(m_frame_count);

    // Record internal command buffer (graphics queue) for texture transitions + descriptor copies
    const auto number_of_textures_to_transition = m_texture_queue.size_approx();
    const bool needs_transition = number_of_textures_to_transition > 0;

    // Descriptor copies
    // TODO: Do it on compute queue?
    m_descriptor_copier.merge_regions();
    const auto descriptor_regions = m_descriptor_copier.get_merged_regions();
    const auto descriptor_region_count = descriptor_regions.size();
    const bool needs_descriptor_copies = descriptor_region_count > 0;

    VkCommandBuffer internal_cmd = VK_NULL_HANDLE;
    if (needs_transition || needs_descriptor_copies)
    {
        auto& internal_pool = acquire_command_pool(GRAPHICS);
        if (internal_pool.command_pool == VK_NULL_HANDLE)
        {
            return false;
        }

        internal_pool.reset();

        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        };

        internal_cmd = internal_pool.create_command_buffer(alloc_info);
        if (internal_cmd == VK_NULL_HANDLE)
        {
            return false;
        }
        set_debug_name(m_device.device,
                       VK_OBJECT_TYPE_COMMAND_BUFFER,
                       reinterpret_cast<uint64_t>(internal_cmd),
                       "Internal Command Buffer");

        constexpr VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        if (VK_FAILED(vkBeginCommandBuffer(internal_cmd, &begin_info)))
        {
            return false;
        }

        if (needs_transition)
        {
            auto const textures = arena.alloc_array<Texture*>(number_of_textures_to_transition);
            size_t texture_count = 0;
            while (texture_count == 0)
            {
                texture_count = m_texture_queue.try_dequeue_bulk(textures, number_of_textures_to_transition);
            }
            assert(texture_count == number_of_textures_to_transition);

            const auto image_barriers = arena.alloc_array<VkImageMemoryBarrier2>(texture_count);
            for (size_t i = 0; i < texture_count; i++)
            {
                const auto texture = textures[i];
                const auto vulkan_texture = to_internal(*texture);
                // Finish before everything else
                image_barriers[i] = {
                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                    .srcAccessMask = 0,
                    .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                    .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
                    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout = vulkan_texture->initial_layout,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image = vulkan_texture->image,
                    .subresourceRange =
                        {
                            .aspectMask = get_image_aspect_mask(convert_format(texture->desc.format)),
                            .baseMipLevel = 0,
                            .levelCount = VK_REMAINING_MIP_LEVELS,
                            .baseArrayLayer = 0,
                            .layerCount = VK_REMAINING_ARRAY_LAYERS,
                        },
                };
            }
            const VkDependencyInfo dep_info{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .imageMemoryBarrierCount = static_cast<uint32_t>(texture_count),
                .pImageMemoryBarriers = image_barriers,
            };
            vkCmdPipelineBarrier2(internal_cmd, &dep_info);
        }

        if (needs_descriptor_copies)
        {
            auto vk_regions = arena.alloc_array<VkBufferCopy2>(descriptor_region_count);
            for (size_t i = 0; i < descriptor_region_count; i++)
            {
                const PendingDescriptorCopy& pending = descriptor_regions[i];

                assert(pending.src.heap->desc.type == pending.dst.heap->desc.type);
                const auto descriptor_size = pending.src.heap->desc.type == DescriptorHeapDesc::Type::SAMPLER
                                               ? m_bloated_sampler_descriptor_size
                                               : m_bloated_resource_descriptor_size;

                const auto src_reserved = get_reserved_range(pending.src.heap->desc.type);
                const auto dst_reserved = get_reserved_range(pending.dst.heap->desc.type);
                vk_regions[i] = {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                    .srcOffset = pending.src.offset * descriptor_size + src_reserved,
                    .dstOffset = pending.dst.offset * descriptor_size + dst_reserved,
                    .size = (pending.descriptors * descriptor_size),
                };
            }

            uint32_t run_start = 0;
            while (run_start < descriptor_region_count)
            {
                const VkBuffer src_buffer = to_internal(*descriptor_regions[run_start].src.heap)->get_buffer();
                const VkBuffer dst_buffer = to_internal(*descriptor_regions[run_start].dst.heap)->get_buffer();

                uint32_t run_end = run_start + 1;
                while (run_end < descriptor_region_count &&
                       to_internal(*descriptor_regions[run_end].src.heap)->get_buffer() == src_buffer &&
                       to_internal(*descriptor_regions[run_end].dst.heap)->get_buffer() == dst_buffer)
                {
                    ++run_end;
                }

                const VkCopyBufferInfo2 copy_info{
                    .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                    .srcBuffer = src_buffer,
                    .dstBuffer = dst_buffer,
                    .regionCount = run_end - run_start,
                    .pRegions = &vk_regions[run_start],
                };
                vkCmdCopyBuffer2(internal_cmd, &copy_info);

                run_start = run_end;
            }

            // Finish before everything else
            const VkMemoryBarrier2 transfer_barrier{
                .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
            };
            const VkDependencyInfo barrier_dep{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .memoryBarrierCount = 1,
                .pMemoryBarriers = &transfer_barrier,
            };
            vkCmdPipelineBarrier2(internal_cmd, &barrier_dep);

            m_descriptor_copier.reset();
        }

        if (VK_FAILED(vkEndCommandBuffer(internal_cmd)))
        {
            return false;
        }
    }

    const bool has_internal_cmd = internal_cmd != VK_NULL_HANDLE;
    // Track the semaphore value so the recycling check knows when this is done
    if (has_internal_cmd)
    {
        ++m_internal_semaphore_value;
    }

    // If non-graphics: Submit separately on the graphics queue and synchronize
    // Otherwise avoid extra submission by adding into the same submit
    if (has_internal_cmd && queue != GRAPHICS)
    {
        const VkCommandBufferSubmitInfo internal_cmd_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = internal_cmd,
        };
        const VkSemaphoreSubmitInfo internal_signal{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_internal_semaphore,
            .value = m_internal_semaphore_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        };
        const VkSubmitInfo2 internal_submit{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &internal_cmd_info,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &internal_signal,
        };
        if (VK_FAILED(vkQueueSubmit2(m_graphics_queue.queue, 1, &internal_submit, VK_NULL_HANDLE)))
        {
            return false;
        }
    }

    // +1 for internal ordering, +1 optional swapchain, +1 optional internal semaphore
    const bool wait_on_internal_semaphore = has_internal_cmd && queue != GRAPHICS;
    uint32_t additional_waits = 1;
    if (submit_info.wait_swapchain)
    {
        ++additional_waits;
    }
    if (wait_on_internal_semaphore)
    {
        ++additional_waits;
    }

    const uint32_t wait_size = submit_info.wait_fence_count + additional_waits;
    const auto wait_semaphore_infos = arena.alloc_array<VkSemaphoreSubmitInfo>(wait_size);

    const VkPipelineStageFlags2 stage_mask = queue == GRAPHICS ? VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
                                           : queue == COMPUTE  ? VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
                                           : queue == COPY     ? VK_PIPELINE_STAGE_2_COPY_BIT
                                                               : VK_PIPELINE_STAGE_2_NONE;
    assert(stage_mask);

    for (unsigned i = 0; i < submit_info.wait_fence_count; i++)
    {
        wait_semaphore_infos[i] = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = *to_internal(submit_info.wait_fences[i]),
            .value = submit_info.wait_values[i],
            .stageMask = stage_mask,
        };
    }
    auto& q = get_queue(queue);
    uint32_t wait_idx = submit_info.wait_fence_count;
    wait_semaphore_infos[wait_idx++] = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = q.semaphore,
        .value = q.last_signaled_fence_value,
        .stageMask = stage_mask,
    };
    if (wait_on_internal_semaphore)
    {
        wait_semaphore_infos[wait_idx++] = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_internal_semaphore,
            .value = m_internal_semaphore_value,
            .stageMask = stage_mask,
        };
    }
    if (submit_info.wait_swapchain)
    {
        wait_semaphore_infos[wait_idx++] = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_image_available_semaphores[m_frame_count % m_image_available_semaphores.size()],
            .value = 0,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
    }

    const bool prepend_internal = has_internal_cmd && queue == GRAPHICS;
    const uint32_t total_cmd_count = submit_info.command_list_count + (prepend_internal ? 1u : 0u);
    const auto cmd_buffer_infos = arena.alloc_array<VkCommandBufferSubmitInfo>(total_cmd_count);
    uint32_t cmd_idx = 0;
    if (prepend_internal)
    {
        cmd_buffer_infos[cmd_idx++] = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = internal_cmd,
        };
    }
    for (unsigned i = 0; i < submit_info.command_list_count; i++)
    {
        cmd_buffer_infos[cmd_idx++] = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = to_internal(submit_info.command_lists[i])->cmd_buf,
        };
    }

    // +1 internal ordering, +1 optional swapchain, +1 optional internal semaphore
    const bool signal_internal_semaphore = prepend_internal;
    const uint32_t max_signal_count = submit_info.signal_fence_count + 1 + (submit_info.signal_swapchain ? 1u : 0u) +
                                      (signal_internal_semaphore ? 1u : 0u);
    const auto signal_semaphore_infos = arena.alloc_array<VkSemaphoreSubmitInfo>(max_signal_count);
    uint32_t signal_count = 0;
    for (unsigned i = 0; i < submit_info.signal_fence_count; i++)
    {
        const auto vk_fence = to_internal(submit_info.signal_fences[i]);
        uint64_t current_val = 0;
        vkGetSemaphoreCounterValue(m_device.device, *vk_fence, &current_val);
        if (submit_info.signal_values[i] <= current_val)
        {
            // No-op
            continue;
        }
        signal_semaphore_infos[signal_count++] = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = *vk_fence,
            .value = submit_info.signal_values[i],
            .stageMask = stage_mask,
        };
    }
    signal_semaphore_infos[signal_count++] = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = q.semaphore,
        .value = ++q.last_signaled_fence_value,
        .stageMask = stage_mask,
    };
    if (signal_internal_semaphore)
    {
        signal_semaphore_infos[signal_count++] = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_internal_semaphore,
            .value = m_internal_semaphore_value,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        };
    }
    if (submit_info.signal_swapchain)
    {
        signal_semaphore_infos[signal_count++] = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_render_finished_semaphores[m_swapchain_index],
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        };
    }

    const VkSubmitInfo2 submit{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = wait_size,
        .pWaitSemaphoreInfos = wait_semaphore_infos,
        .commandBufferInfoCount = total_cmd_count,
        .pCommandBufferInfos = cmd_buffer_infos,
        .signalSemaphoreInfoCount = signal_count,
        .pSignalSemaphoreInfos = signal_semaphore_infos,
    };

    if (VK_FAILED(vkQueueSubmit2(q.queue, 1, &submit, VK_NULL_HANDLE)))
    {
        return false;
    }

    return true;
}

bool VulkanContext::create_fence(Fence* fence, const uint64_t initial_value, const char* debug_name)
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

    set_debug_name(m_device.device, VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(*vk_fence), debug_name);

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
        .flags = info.wait_all ? 0u : VK_SEMAPHORE_WAIT_ANY_BIT,
        .semaphoreCount = info.count,
        .pSemaphores = semaphores,
        .pValues = info.values,
    };
    return VK_SUCCEEDED(vkWaitSemaphores(m_device.device, &wait_info, std::numeric_limits<uint64_t>::max()));
}

void VulkanContext::set_barrier_resource(unsigned count, ImageBarrier* barriers, const Swapchain& swapchain)
{
    for (unsigned i = 0; i < count; i++)
    {
        barriers[i].resource = reinterpret_cast<void*>(m_swapchain.images[m_swapchain_index]);
    }
}

void VulkanContext::set_barrier_resource(const unsigned count, ImageBarrier* barriers, const Texture& render_target)
{
    const auto vk_texture = to_internal(render_target);
    for (unsigned i = 0; i < count; i++)
    {
        barriers[i].resource = reinterpret_cast<void*>(vk_texture->image);
    }
}

bool VulkanContext::issue_barrier(CommandList* cmd_list, const unsigned count, const ImageBarrier* barriers)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    auto& arena = acquire_arena(m_frame_count);
    const auto vk_barriers = arena.alloc_array<VkImageMemoryBarrier2>(count);

    for (unsigned i = 0; i < count; i++)
    {
        const auto& barrier = barriers[i];
        if (!barrier.resource)
        {
            return false;
        }

        vk_barriers[i] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = sync_stage(barrier.src_stage),
            .srcAccessMask = access_flags(barrier.src_access),
            .dstStageMask = sync_stage(barrier.dst_stage),
            .dstAccessMask = access_flags(barrier.dst_access),
            .oldLayout = layout(barrier.src_layout),
            .newLayout = layout(barrier.dst_layout),
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = static_cast<VkImage>(barrier.resource),
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = barrier.subresource_range.base_mip_level,
                    .levelCount = barrier.subresource_range.mip_level_count,
                    .baseArrayLayer = barrier.subresource_range.base_array_layer,
                    .layerCount = barrier.subresource_range.array_layer_count,
                },
        };
    }

    const VkDependencyInfo dep_info{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = count,
        .pImageMemoryBarriers = vk_barriers,
    };
    vkCmdPipelineBarrier2(vk_cmd_list->cmd_buf, &dep_info);

    return true;
}

void VulkanContext::init_imgui(const DisplayWindow& window, const Swapchain& swapchain)
{
    ImGui_ImplSDL3_InitForVulkan(window.get_window());

    const VkFormat color_format = convert_format(swapchain.format);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_4;
    init_info.Instance = m_instance.instance;
    init_info.PhysicalDevice = m_device.physical_device;
    init_info.Device = m_device.device;
    init_info.QueueFamily = m_graphics_queue.family_index;
    init_info.Queue = m_graphics_queue.queue;
    init_info.DescriptorPool = VK_NULL_HANDLE;
    init_info.DescriptorPoolSize = 1000;
    init_info.MinImageCount = 2;
    init_info.ImageCount = swapchain.buffer_count;
    init_info.PipelineCache = VK_NULL_HANDLE;

    init_info.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pNext = nullptr;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &color_format;

    init_info.PipelineInfoForViewports.RenderPass = VK_NULL_HANDLE;
    init_info.PipelineInfoForViewports.Subpass = 0;
    init_info.PipelineInfoForViewports.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo.pNext = nullptr;
    init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoForViewports.PipelineRenderingCreateInfo.pColorAttachmentFormats = &color_format;
    init_info.PipelineInfoForViewports.SwapChainImageUsage = 0;

    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;

    ImGui_ImplVulkan_Init(&init_info);
}

void VulkanContext::start_imgui_frame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void VulkanContext::render_imgui_draw_data(CommandList* cmd_list)
{
    const auto vk_cmd_list = to_internal(*cmd_list);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vk_cmd_list->cmd_buf);
}

void VulkanContext::destroy_imgui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
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
    switch (queue)
    {
    case GRAPHICS:
        return VK_SUCCEEDED(vkQueueWaitIdle(m_graphics_queue.queue));
    case COMPUTE:
        return VK_SUCCEEDED(vkQueueWaitIdle(m_compute_queue.queue));
    case COPY:
        return VK_SUCCEEDED(vkQueueWaitIdle(m_transfer_queue.queue));
    default:
        return VK_SUCCEEDED(vkDeviceWaitIdle(m_device.device));
    }
}

VulkanContext::~VulkanContext()
{
    if (m_device.device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(m_device.device);
    }
    vmaDestroyAllocator(m_allocator);
    vkb::destroy_swapchain(m_swapchain.swapchain);
    if (m_surface)
    {
        vkb::destroy_surface(m_instance, m_surface);
    }
    for (const auto& q : {m_graphics_queue, m_compute_queue, m_transfer_queue})
    {
        vkDestroySemaphore(m_device.device, q.semaphore, nullptr);
    }
    for (const auto& sem : m_image_available_semaphores)
    {
        vkDestroySemaphore(m_device.device, sem, nullptr);
    }
    for (const auto& sem : m_render_finished_semaphores)
    {
        vkDestroySemaphore(m_device.device, sem, nullptr);
    }
    vkDestroySemaphore(m_device.device, m_internal_semaphore, nullptr);
    vkb::destroy_device(m_device);
    vkb::destroy_instance(m_instance);
}

VkDeviceSize VulkanContext::get_reserved_range(const DescriptorHeapDesc::Type type) const
{
    if (type == DescriptorHeapDesc::Type::CBV_SRV_UAV)
    {
        return m_capabilities.descriptor_heap_properties.minResourceHeapReservedRange;
    }
    return m_capabilities.descriptor_heap_properties.minSamplerHeapReservedRange;
}

bool VulkanContext::create_descriptor_buffer(const Buffer& buffer,
                                             DescriptorHeap* heap,
                                             Descriptor* const descriptor,
                                             const VkDescriptorType type) const
{
    const auto vk_heap = to_internal(*heap);
    const auto vk_buffer = to_internal(buffer);

    VkDeviceAddressRangeEXT address_range{
        .address = get_gpu_address(m_device.device, vk_buffer->buffer),
        .size = buffer.desc.size,
    };

    const VkResourceDescriptorInfoEXT resource_info{
        .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
        .type = type,
        .data{
            .pAddressRange = &address_range,
        },
    };

    const auto address = vk_heap->get_cpu_pointer(descriptor->offset * m_bloated_resource_descriptor_size);
    const VkHostAddressRangeEXT range{
        .address = address,
        .size = m_capabilities.descriptor_heap_properties.bufferDescriptorSize,
    };

    const auto result = VK_SUCCEEDED(vkWriteResourceDescriptorsEXT(m_device.device, 1, &resource_info, &range));

    if (result)
    {
        descriptor->heap = heap;
        return true;
    }
    return false;
}

bool VulkanContext::create_descriptor_texture(const Texture& texture,
                                              DescriptorHeap* heap,
                                              Descriptor* descriptor,
                                              const VkDescriptorType type) const
{
    const auto vk_heap = to_internal(*heap);
    const auto vk_texture = to_internal(texture);

    const VkFormat vk_format = convert_format(texture.desc.format);
    const VkImageAspectFlags aspect_mask = get_image_aspect_mask(vk_format);

    const VkImageViewCreateInfo view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = vk_texture->image,
        .viewType = view_type_from_desc(texture.desc),
        .format = vk_format,
        .subresourceRange =
            {
                .aspectMask = aspect_mask,
                .baseMipLevel = 0,
                .levelCount = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount = VK_REMAINING_ARRAY_LAYERS,
            },
    };

    const VkImageLayout descriptor_access_layout = (aspect_mask & VK_IMAGE_ASPECT_COLOR_BIT) != 0
                                                     ? layout(Layout::SHADER_RESOURCE)
                                                     : layout(Layout::DEPTH_STENCIL_READ);

    VkImageDescriptorInfoEXT image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
        .pView = &view_info,
        .layout = descriptor_access_layout,
    };

    const VkResourceDescriptorInfoEXT resource_info{
        .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
        .type = type,
        .data{
            .pImage = &image_info,
        },
    };

    const auto address = vk_heap->get_cpu_pointer(descriptor->offset * m_bloated_resource_descriptor_size);
    const VkHostAddressRangeEXT range{
        .address = address,
        .size = m_capabilities.descriptor_heap_properties.imageDescriptorSize,
    };

    const auto result = VK_SUCCEEDED(vkWriteResourceDescriptorsEXT(m_device.device, 1, &resource_info, &range));

    if (result)
    {
        descriptor->heap = heap;
        return true;
    }
    return false;
}

VulkanContext::VulkanQueue& VulkanContext::get_queue(const QueueType queue)
{
    switch (queue)
    {
    case GRAPHICS:
        return m_graphics_queue;
    case COMPUTE:
        return m_compute_queue;
    case COPY:
        return m_transfer_queue;
    default:
        assert(false);
        return m_graphics_queue;
    }
}

VulkanCommandPool& VulkanContext::acquire_command_pool(const QueueType queue)
{
    // One pool per thread that is double buffered
    thread_local std::array<VulkanCommandPool, 2> thread_pools;
    for (size_t i = 0; i < thread_pools.size(); i++)
    {
        const VkCommandPoolCreateInfo internal_pool_ci{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = get_queue(queue).family_index,
        };
        auto& pool = thread_pools[i];
        if (pool.device == VK_NULL_HANDLE)
        {
            VkCommandPool cmd_pool;
            if (VK_FAILED(vkCreateCommandPool(m_device.device, &internal_pool_ci, nullptr, &cmd_pool)))
            {
                pool.command_pool = VK_NULL_HANDLE;
                return pool;
            }
            set_debug_name(m_device.device,
                           VK_OBJECT_TYPE_COMMAND_POOL,
                           reinterpret_cast<uint64_t>(cmd_pool),
                           "Internal Command Pool");

            pool.init(m_device.device, cmd_pool);
        }
    }
    return thread_pools[m_frame_count % thread_pools.size()];
}
