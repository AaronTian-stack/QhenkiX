#pragma once

#include <volk.h>

struct VulkanSemaphore
{
    VkDevice device;
    VkSemaphore semaphore;
    VulkanSemaphore(const VkDevice device, const VkSemaphore semaphore)
        : device(device),
          semaphore(semaphore)
    {
    }
    VulkanSemaphore(const VulkanSemaphore&) = delete;
    VulkanSemaphore(VulkanSemaphore&&) = delete;
    VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;
    VulkanSemaphore& operator=(VulkanSemaphore&&) = delete;
    ~VulkanSemaphore()
    {
        vkDestroySemaphore(device, semaphore, nullptr);
    }
};
