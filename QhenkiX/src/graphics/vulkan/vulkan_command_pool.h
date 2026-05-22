#pragma once

#include <volk.h>

#include <array>

namespace qhenki::gfx
{
class VulkanCommandPool
{
    VkDevice m_device = VK_NULL_HANDLE;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;
    // You shouldn't be submitting so many command buffers
    std::array<VkCommandBuffer, 32> m_command_buffers{};
    unsigned m_command_buffer_count = 0;

public:
    VulkanCommandPool() = default;

    // Expects pool to already be created
    VulkanCommandPool(VkDevice device, VkCommandPool pool);
    VulkanCommandPool(const VulkanCommandPool&) = delete;
    VulkanCommandPool(VulkanCommandPool&&) = delete;
    VulkanCommandPool& operator=(const VulkanCommandPool&) = delete;
    VulkanCommandPool& operator=(VulkanCommandPool&&) = delete;
    ~VulkanCommandPool();

    void init(VkDevice device, VkCommandPool pool);
    bool is_valid() const;

    VkCommandBuffer create_command_buffer(VkCommandBufferAllocateInfo& info);
    VkResult reset();
};
} // namespace qhenki::gfx
