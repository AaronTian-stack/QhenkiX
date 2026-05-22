#pragma once

#include <volk.h>

#include "vulkan_root_signature.h"

namespace qhenki::gfx
{
struct VulkanPipeline
{
    VkDevice device;
    VkPipeline pipeline;
    VkPrimitiveTopology topology;
    VulkanRootSignature* root_signature;
    VulkanPipeline() = default;
    VulkanPipeline(const VkDevice device,
                   const VkPipeline pipeline,
                   const VkPrimitiveTopology topology,
                   VulkanRootSignature* const root_signature)
        : device(device),
          pipeline(pipeline),
          topology(topology),
          root_signature(root_signature)
    {
    }
    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline(VulkanPipeline&&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(VulkanPipeline&&) = delete;
    ~VulkanPipeline()
    {
        vkDestroyPipeline(device, pipeline, nullptr);
    }
};
} // namespace qhenki::gfx
