#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace qhenki::gfx
{
struct VulkanShader
{
    VkShaderModule module;
    std::vector<uint32_t> spirv;
};
} // namespace qhenki::gfx
