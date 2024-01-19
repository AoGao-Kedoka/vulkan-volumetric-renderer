#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "buffer.h"
#include "config.h"
#include "core.h"
#include "texture.h"
#include "ui.h"

struct UniformBufferObject {
    float deltaTime = 1.0f;
};

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2>
    getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2>
            attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
    }
};

class Application {
public:
    void run()
    {
        initWindow();
        initVulkan();
        uiInterface.Init(2, renderPass);
        mainLoop();
        cleanup();
    }

private:
    void initWindow();
    void initVulkan();
    void cleanup();
    void mainLoop();
    void recreateSwapChain();
    void createInstance();
    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setupDebugMessenger();
    void createSurface();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createComputeDescriptorSetLayout();
    void createGraphicsDescriptorSetLayout();
    void createGraphicsPipeline();
    void createComputePipeline();
    void createFramebuffers();
    void createCommandPool();
    void createShaderStorageBuffers();
    void createUniformBuffers();
    void createDescriptorPool();
    void createComputeDescriptorSets();
    void createCommandBuffers();
    void createComputeCommandBuffers();
    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);
    void createSyncObjects();
    void drawFrame();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    std::vector<const char *> getRequiredExtensions();

    static void framebufferResizeCallback(GLFWwindow *window, int width,
                                          int height)
    {
        auto app =
            reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void cleanupSwapChain()
    {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(core.device, framebuffer, nullptr);
        }

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(core.device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(core.device, swapChain, nullptr);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = core.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        core.endSingleTimeCommands(commandBuffer);
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        UniformBufferObject ubo{};
        ubo.deltaTime = lastFrameTime * 2.0f;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

private:
    Core core;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorSetLayout computeDescriptorSetLayout;
    VkDescriptorSetLayout graphicsDescriptorSetLayout;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;

    std::vector<Buffer> shaderStorageBuffers;

    std::vector<Buffer> uniformBuffers;
    std::vector<void *> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> computeDescriptorSets;
    std::vector<VkDescriptorSet> graphicsDescriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> computeCommandBuffers;
    Texture computeStoragetexture;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> computeInFlightFences;
    uint32_t currentFrame = 0;

    float lastFrameTime = 0.0f;
    bool framebufferResized = false;
    double lastTime = 0.0f;

    UserInterface uiInterface { &core };
};
