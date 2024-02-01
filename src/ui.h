#pragma once

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>
#include <fmt/format.h>

#include <array>

#include "core.h"
#include "config.h"

class UserInterface {
public:
    explicit UserInterface(Core *core) : core{core} {};
    void Init(uint32_t imageCount, VkRenderPass& renderPass);
    void Render();
    void RecordToCommandBuffer(VkCommandBuffer commandBuffer);
    void Cleanup();

    std::array<float, 3> GetSunPositionFromUIInput()
    {
        std::array<float, 3> res{};
        std::copy(std::begin(uiSunPosition), std::end(uiSunPosition),
                  std::begin(res));
        return res;
    }
    std::array<float, 3> GetWindDirectionFromUIInput()
    {
        std::array<float, 3> res{};
        std::copy(std::begin(uiWindDirection), std::end(uiWindDirection),
                  std::begin(res));
        return res;
    }

private:
    Core *core;
    VkDescriptorPool imguiPool{};

    float uiSunPosition[3] = {2.0f, -10.0f, 4.0f};
    float uiWindDirection[3] = {1, -0.2, 1.0};
};
