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
#include <cmath>

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

static float rotatingAngle = 0;
static double lastX = 0.0;
static double lastY = 0.0;
static bool isDragging = false;

static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
        isDragging = true;
        glfwGetCursorPos(window, &lastX, &lastY);
    } else if (action == GLFW_RELEASE) {
        isDragging = false;
    }
    }
}

// Cursor position callback function
static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging) {
        double deltaX = xpos - lastX;
        double deltaY = ypos - lastY;
        lastX = xpos;
        lastY = ypos;
        rotatingAngle += deltaX * 0.005;
    }
}

class Application {
public:
    ~Application() { glfwTerminate();}
    void run()
    {
        initWindow();
        initVulkan();
        uiInterface.Init(2, renderPass);
        mainLoop();
        cleanup();
    }

private:
    //----------------------------------------------------
    // Initialization
    //----------------------------------------------------
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
    void createComputePipeline(std::string computeShaderPath,
                               VkPipelineLayout& computeLayout,
                               VkPipeline& computePipeline);
    void createFramebuffers();
    void createCommandPool();
    void createShaderStorageBuffers();
    void createUniformBuffers();
    void createDescriptorPool();
    void createComputeDescriptorSets();
    void createGraphicsDescriptorSets();
    void createCommandBuffers();
    void createComputeCommandBuffers();
    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer);
    void recordCommandBuffer(VkCommandBuffer commandBuffer,
                             uint32_t imageIndex);
    void createSyncObjects();
    //----------------------------------------------------
    // Rendering
    //----------------------------------------------------
    void drawFrame();
    [[nodiscard]] VkShaderModule createShaderModule(
        const std::vector<char>& code) const;
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    [[nodiscard]] VkExtent2D chooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities) const;
    static std::vector<const char *> getRequiredExtensions();

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

    //----------------------------------------------------
    // Physics
    //----------------------------------------------------

    void UpdateParticle(std::vector<Particle>& particles);

    //----------------------------------------------------
    // Util
    //----------------------------------------------------
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = core.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        core.endSingleTimeCommands(commandBuffer);
    }

    bool isKeyPressed(GLFWwindow* window, int key) {
        return glfwGetKey(window, key) == GLFW_PRESS;
    }

    void updateUniformBuffer(uint32_t currentImage)
    {
        UniformBufferObject ubo{};
        ubo.deltaTime = lastFrameTime * 2.0f;
        ubo.totalTime = static_cast<float_t>(glfwGetTime());
        ubo.sunPosition = glm::vec3(uiInterface.GetSunPositionFromUIInput()[0], uiInterface.GetSunPositionFromUIInput()[1] - 5, uiInterface.GetSunPositionFromUIInput()[2]);
        ubo.frame = frames;
        ubo.windDirection =
            glm::vec3(uiInterface.GetWindDirectionFromUIInput()[0],
                      uiInterface.GetWindDirectionFromUIInput()[1],
                      uiInterface.GetWindDirectionFromUIInput()[2]);
        ubo.particleBasedFluid = uiInterface.GetParticleBasedFluid();
        if (!ubo.particleBasedFluid && core.CurrentPipeline == 0)
            ubo.rotationY = rotatingAngle;
        else {
            ubo.rotationY = 0;
        }

        if (isKeyPressed(core.window, GLFW_KEY_W)) {
            cameraPos += glm::vec3(0, 0, -0.01);
        }
        if (isKeyPressed(core.window, GLFW_KEY_S)) {
            cameraPos += glm::vec3(0, 0, 0.01);
        }
        if (isKeyPressed(core.window, GLFW_KEY_LEFT_CONTROL)) {
            cameraPos += glm::vec3(0, 0.01, 0);
        }
        if (isKeyPressed(core.window, GLFW_KEY_SPACE)) {
            cameraPos += glm::vec3(0, -0.01, 0);
        }
        if (isKeyPressed(core.window, GLFW_KEY_A)) {
            cameraPos += glm::vec3(-0.01, 0, 0);
        }
        if (isKeyPressed(core.window, GLFW_KEY_D)) {
            cameraPos += glm::vec3(0.01, 0, 0);
        }

        ubo.cameraPosition = cameraPos;

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

    VkDescriptorSetLayout computeDescriptorSetLayout;
    VkDescriptorSetLayout graphicsDescriptorSetLayout;

    VkPipelineLayout computeFluidPipelineLayout; // pipeline flag 0
    VkPipeline computeFluidPipeline;
    VkPipelineLayout computeSmokePipelineLayout; // pipeline flag 1
    VkPipeline computeSmokePipeline;

    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;

    std::vector<Buffer> shaderStorageBuffers;

    glm::vec3 cameraPos = glm::vec3(0, 0, 10);
    std::vector<Buffer> uniformBuffers;
    std::vector<void *> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> computeDescriptorSets;
    std::vector<VkDescriptorSet> graphicsDescriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> computeCommandBuffers;
    Texture computeStorageTexture;
    Texture causticTexture;
    Texture computeCloudNoiseTexture;
    Texture computeCloudBlueNoiseTexture;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> computeInFlightFences;
    uint32_t currentFrame = 0;
    uint32_t frames = 0;

    float lastFrameTime = 0.0f;
    bool framebufferResized = false;
    double lastTime = 0.0f;

    UserInterface uiInterface{&core};

    std::vector<Particle> particles;
};
