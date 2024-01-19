#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>

#include "core.h"

class Buffer {
public:
    Buffer(Core* core, VkDeviceSize size, VkBufferUsageFlags usage,
           VkMemoryPropertyFlags properties);
    VkBuffer GetBuffer() { return buffer; }
    VkDeviceMemory GetDeviceMemory() { return bufferMemory; }
    void Cleanup();

private:
    void CheckValue()
    {
        if (buffer == VK_NULL_HANDLE || bufferMemory == VK_NULL_HANDLE)
            std::runtime_error("Something went wrong with buffer");
    }

    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties)
    {
        CheckValue();
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(core->physicalDevice,
                                            &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) ==
                    properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    Core* core;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
};
