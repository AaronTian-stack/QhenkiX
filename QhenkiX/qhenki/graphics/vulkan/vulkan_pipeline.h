#pragma once

#include "vulkan_root_signature.h"

struct VulkanPipeline
{
    VkPipeline pipeline;
    VkPrimitiveTopology topology;
    qhenki::gfx::VulkanRootSignature* root_signature;
};
