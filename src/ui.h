#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#include <array>

class UserInterface {
public:
    void Init(VkDevice device, GLFWwindow *window, VkInstance instance,
              VkPhysicalDevice physicalDevice, VkQueue graphicsQueue,
              uint32_t imageCount, VkRenderPass renderPass);
    void Render();
    void RecordToCommandBuffer(VkCommandBuffer commandBuffer);
    void Cleanup();

private:
    VkDescriptorPool imguiPool;
    VkDevice device;
};
