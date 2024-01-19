#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>
#include <array>


#include "core.h"

class UserInterface {
public:
    explicit UserInterface(Core* core):core{core}{};
    void Init(uint32_t imageCount, VkRenderPass &renderPass);
    void Render();
    void RecordToCommandBuffer(VkCommandBuffer commandBuffer);
    void Cleanup();

private:
    Core* core;
    VkDescriptorPool imguiPool{};
};
