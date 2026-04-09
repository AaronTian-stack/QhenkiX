#pragma once

#include "vulkan_root_signature.h"

namespace qhenki::gfx
{
struct VulkanPipeline
{
    VkPipeline pipeline;
    VkPrimitiveTopology topology;
    VulkanRootSignature* root_signature;
};
} // namespace qhenki::gfx
