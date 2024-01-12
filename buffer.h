#pragma once

#include <vulkan/vulkan_core.h>

class Buffer
{
public:
	Buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
private:
	VkBuffer buffer;
};
