#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <vulkan/vulkan.h>

#include <vector>

namespace qhenki::gfx
{
struct VulkanRootSignature
{
    std::vector<VkDescriptorSetAndBindingMappingEXT> bindings;
    VkShaderDescriptorSetAndBindingMappingInfoEXT layout;
    uint32_t push_range_count = 0;
};
} // namespace qhenki::gfx
