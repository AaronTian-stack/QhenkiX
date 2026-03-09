#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace qhenki::gfx
{
struct VulkanRootSignature
{
    std::vector<VkDescriptorSetAndBindingMappingEXT> bindings;
    VkShaderDescriptorSetAndBindingMappingInfoEXT layout;
};
} // namespace qhenki::gfx
