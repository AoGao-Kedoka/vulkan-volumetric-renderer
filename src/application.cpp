#include "application.h"

uint32_t WIDTH = 800;
uint32_t HEIGHT = 600;
const uint32_t PARTICLE_COUNT = 5;

const int MAX_FRAMES_IN_FLIGHT = 2;

const float boxMinX = -2.0;
const float boxMaxX = 2.0;
const float boxMinY = 0;
const float boxMaxY = 2.0;
const float boxMinZ = -2.0;
const float boxMaxZ = 2.0;



void Application::initWindow()
{
    glfwInit();

    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    WIDTH = mode->width * 0.5f;
    HEIGHT = mode->height * 0.5f;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    core.window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Volumetric Renderer",
                                   nullptr, nullptr);
    glfwSetWindowUserPointer(core.window, this);
    glfwSetFramebufferSizeCallback(core.window, framebufferResizeCallback);
    glfwSetMouseButtonCallback(core.window, mouseCallback);

    glfwSetCursorPosCallback(core.window, cursorPositionCallback);
}

void Application::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    core.CreateDevices();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createComputeDescriptorSetLayout();
    createGraphicsDescriptorSetLayout();
    createComputePipeline(FilePath::computeFluidShaderPath,
                          computeFluidPipelineLayout, computeFluidPipeline);
    createComputePipeline(FilePath::computeSmokeShaderPath,
                          computeSmokePipelineLayout, computeSmokePipeline);
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createShaderStorageBuffers();
    createUniformBuffers();
    createDescriptorPool();
    createComputeDescriptorSets();
    createGraphicsDescriptorSets();
    createCommandBuffers();
    createComputeCommandBuffers();
    createSyncObjects();
}

void Application::cleanup()
{
    cleanupSwapChain();

    uiInterface.Cleanup();

    computeStorageTexture.Cleanup();
    causticTexture.Cleanup();
    computeCloudNoiseTexture.Cleanup();
    computeCloudBlueNoiseTexture.Cleanup();

    vkDestroyPipeline(core.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(core.device, graphicsPipelineLayout, nullptr);

    vkDestroyPipeline(core.device, computeFluidPipeline, nullptr);
    vkDestroyPipelineLayout(core.device, computeFluidPipelineLayout, nullptr);
    vkDestroyPipeline(core.device, computeSmokePipeline, nullptr);
    vkDestroyPipelineLayout(core.device, computeSmokePipelineLayout, nullptr);

    vkDestroyRenderPass(core.device, renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i].Cleanup();
    }

    vkDestroyDescriptorPool(core.device, descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(core.device, computeDescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorSetLayout(core.device, graphicsDescriptorSetLayout,
                                 nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        shaderStorageBuffers[i].Cleanup();
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(core.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(core.device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(core.device, computeFinishedSemaphores[i], nullptr);
        vkDestroyFence(core.device, inFlightFences[i], nullptr);
        vkDestroyFence(core.device, computeInFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(core.device, core.commandPool, nullptr);

    vkDestroyDevice(core.device, nullptr);

    if (enableValidationLayers) {
        Core::DestroyDebugUtilsMessengerEXT(core.instance, debugMessenger,
                                            nullptr);
    }

    vkDestroySurfaceKHR(core.instance, core.surface, nullptr);
    vkDestroyInstance(core.instance, nullptr);

    glfwDestroyWindow(core.window);
}

void Application::mainLoop()
{
    while (!glfwWindowShouldClose(core.window)) {
        glfwPollEvents();
        if (core.CurrentPipeline == 1) UpdateParticle(particles);
        uiInterface.Render();
        drawFrame();
        double currentTime = glfwGetTime();
        lastFrameTime = (currentTime - lastTime) * 1000.0;
        lastTime = currentTime;
    }

    vkDeviceWaitIdle(core.device);
}

void Application::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(core.window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(core.window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(core.device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
}

void Application::createInstance()
{
    if (enableValidationLayers && !Core::CheckValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Volumetric Renderer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        // createInfo.pNext =
        //     (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;

        VkValidationFeatureEnableEXT enabled[] = {
            VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT};
        VkValidationFeaturesEXT features{
            VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT};
        features.disabledValidationFeatureCount = 0;
        features.enabledValidationFeatureCount = 1;
        features.pDisabledValidationFeatures = nullptr;
        features.pEnabledValidationFeatures = enabled;
        createInfo.pNext = &features;

        populateDebugMessengerCreateInfo(debugCreateInfo);
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &core.instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create core.instance!");
    }

    std::cout << "[INFO] Vulkan instance created..." << std::endl;
}

void Application::populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = Core::DebugCallback;
}

void Application::setupDebugMessenger()
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (Core::CreateDebugUtilsMessengerEXT(core.instance, &createInfo, nullptr,
                                           &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void Application::createSurface()
{
    if (glfwCreateWindowSurface(core.instance, core.window, nullptr,
                                &core.surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create core.window surface!");
    }

    std::cout << "[INFO] Vulkan surface created..." << std::endl;
}

void Application::createSwapChain()
{
    Core::SwapChainSupportDetails swapChainSupport =
        core.QuerySwapChainSupport(core.physicalDevice);

    VkSurfaceFormatKHR surfaceFormat =
        chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode =
        chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 &&
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = core.surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    Core::QueueFamilyIndices indices =
        core.FindQueueFamilies(core.physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(),
                                     indices.presentFamily.value()};

    if (indices.graphicsAndComputeFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // Optional
        createInfo.pQueueFamilyIndices = nullptr;  // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(core.device, &createInfo, nullptr, &swapChain) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    std::cout << "[INFO] Vulkan swapchain created..." << std::endl;

    vkGetSwapchainImagesKHR(core.device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(core.device, swapChain, &imageCount,
                            swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
    std::cout << "[INFO] Vulkan swapchain created..." << std::endl;
}

void Application::createImageViews()
{
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(core.device, &createInfo, nullptr,
                              &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void Application::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(core.device, &renderPassInfo, nullptr,
                           &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
    std::cout << "[INFO] Vulkan render pass created..." << std::endl;
}

void Application::createComputeDescriptorSetLayout()
{
    std::array<VkDescriptorSetLayoutBinding, 6> layoutBindings{};

    // Storage texture
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Time vector uniform buffer
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Particles data storage buffer
    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[2].pImmutableSamplers = nullptr;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Noise texture sampler
    layoutBindings[3].binding = 3;
    layoutBindings[3].descriptorCount = 1;
    layoutBindings[3].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[3].pImmutableSamplers = nullptr;
    layoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Blue Noise texture sampler
    layoutBindings[4].binding = 4;
    layoutBindings[4].descriptorCount = 1;
    layoutBindings[4].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[4].pImmutableSamplers = nullptr;
    layoutBindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    // Blue Noise texture sampler
    layoutBindings[5].binding = 5;
    layoutBindings[5].descriptorCount = 1;
    layoutBindings[5].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[5].pImmutableSamplers = nullptr;
    layoutBindings[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = layoutBindings.size();
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(core.device, &layoutInfo, nullptr,
                                    &computeDescriptorSetLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create compute descriptor set layout!");
    }

    std::cout << "[INFO] Vulkan comptute descriptor set layout created..."
              << std::endl;
}

void Application::createGraphicsDescriptorSetLayout()
{
    // 0: Texture from compute shader, 1: Caustic texture
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{};

    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[0].pImmutableSamplers = nullptr;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[1].pImmutableSamplers = nullptr;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutInfo.pBindings = layoutBindings.data();

    if (vkCreateDescriptorSetLayout(core.device, &layoutInfo, nullptr,
                                    &graphicsDescriptorSetLayout) !=
        VK_SUCCESS) {
        throw std::runtime_error(
            "failed to create compute descriptor set layout!");
    }
    std::cout << "[INFO] Vulkan graphics descriptor set layout created..."
              << std::endl;
}

void Application::createGraphicsPipeline()
{
    auto vertShaderCode = Core::ReadFile(FilePath::vertexShaderPath);
    auto fragShaderCode = Core::ReadFile(FilePath::fragmentShaderPath);

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                      fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pSetLayouts = &graphicsDescriptorSetLayout;

    if (vkCreatePipelineLayout(core.device, &pipelineLayoutInfo, nullptr,
                               &graphicsPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = graphicsPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(core.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(core.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(core.device, vertShaderModule, nullptr);
    std::cout << "[INFO] Vulkan graphics pipeline created..." << std::endl;
}

void Application::createComputePipeline(std::string computeShaderPath,
                                        VkPipelineLayout& computeLayout,
                                        VkPipeline& computePipeline)
{
    auto computeShaderCode = Core::ReadFile(computeShaderPath);
    VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;                         // Optional
    pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;  // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;                 // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;              // Optional

    if (vkCreatePipelineLayout(core.device, &pipelineLayoutInfo, nullptr,
                               &computeLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = computeLayout;
    pipelineInfo.stage = computeShaderStageInfo;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // Optional
    pipelineInfo.basePipelineIndex = -1;               // Optional

    if (vkCreateComputePipelines(core.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                 nullptr, &computePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

    vkDestroyShaderModule(core.device, computeShaderModule, nullptr);
    std::cout << "[INFO] Vulkan compute pipeline created..." << std::endl;
}

void Application::createFramebuffers()
{
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(core.device, &framebufferInfo, nullptr,
                                &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
    std::cout << "[INFO] Vulkan frame buffer created..." << std::endl;
}

void Application::createCommandPool()
{
    Core::QueueFamilyIndices queueFamilyIndices =
        core.FindQueueFamilies(core.physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex =
        queueFamilyIndices.graphicsAndComputeFamily.value();

    if (vkCreateCommandPool(core.device, &poolInfo, nullptr,
                            &core.commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
    std::cout << "[INFO] Vulkan command pool created..." << std::endl;
}

void Application::createShaderStorageBuffers()
{
    // Initialize particles
    //std::default_random_engine rndEngine((unsigned)time(nullptr));
    //std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(-0.1, 0.1);

    // Initial particle positions on a circle
    particles.resize(PARTICLE_COUNT);
    int z = 0;
    float stepX = 0.1;
    for (auto& particle : particles) {
        // float r = 0.25f * sqrt(rndDist(rndEngine));
        // float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
        // float x = r * cos(theta) * HEIGHT / WIDTH;
        // float y = r * sin(theta);
        // int s = z % 4 + 1;
        // particle.position = glm::vec4(x, y, z, (6 - s) * .25);
        // particle.velocity = glm::normalize(glm::vec3(x, y, z)) * 0.00025f;
        // particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine),
        //                            rndDist(rndEngine), 1.0f);
        particle.position = glm::vec4(-0.3 + z *stepX, 1 ,0,1);
        particle.velocity = glm::vec3(dist(mt), dist(mt), dist(mt));
        particle.color = glm::vec4(1, 1, 1, 1);
        ++z;
    }

    VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

    // Create a staging buffer used to upload data to the gpu
    Buffer stagingBuffer{&core, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

    void *data;
    vkMapMemory(core.device, stagingBuffer.GetDeviceMemory(), 0, bufferSize, 0,
                &data);
    memcpy(data, particles.data(), (size_t)bufferSize);
    vkUnmapMemory(core.device, stagingBuffer.GetDeviceMemory());

    // Copy initial particle data to all storage buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        Buffer shaderStorageBuffer{&core, bufferSize,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};
        copyBuffer(stagingBuffer.GetBuffer(), shaderStorageBuffer.GetBuffer(),
                   bufferSize);
        shaderStorageBuffers.push_back(shaderStorageBuffer);
    }

    stagingBuffer.Cleanup();
}

void Application::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        Buffer uniformBuffer{
            &core,
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        vkMapMemory(core.device, uniformBuffer.GetDeviceMemory(), 0, bufferSize,
                    0, &uniformBuffersMapped[i]);

        uniformBuffers.push_back(uniformBuffer);
    }
}

void Application::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount =
        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[2].descriptorCount = 1000;

    poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[3].descriptorCount = 1000;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 10000;

    if (vkCreateDescriptorPool(core.device, &poolInfo, nullptr,
                               &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
    std::cout << "[INFO] Vulkan descriptor pool created..." << std::endl;
}

void Application::createComputeDescriptorSets()
{
    computeStorageTexture =
        Texture{&core, WIDTH, HEIGHT, VK_FORMAT_R8G8B8A8_UNORM}
            .CreateImageView()
            .CreateImageSampler();
    computeStorageTexture.TransitionImageLayout(
        computeStorageTexture.GetImage(), computeStorageTexture.GetFormat(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    computeCloudNoiseTexture = Texture{
        &core, FilePath::computeCloudNoiseTexturePath, VK_FORMAT_R8G8B8A8_SRGB};
    computeCloudNoiseTexture.CreateImageView().CreateImageSampler();
    computeCloudNoiseTexture.TransitionImageLayout(
        computeCloudNoiseTexture.GetImage(),
        computeCloudNoiseTexture.GetFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    computeCloudBlueNoiseTexture =
        Texture{&core, FilePath::computeCloudBlueNoiseTexturePath,
                VK_FORMAT_R8G8B8A8_SRGB};
    computeCloudBlueNoiseTexture.CreateImageView().CreateImageSampler();
    computeCloudBlueNoiseTexture.TransitionImageLayout(
        computeCloudBlueNoiseTexture.GetImage(),
        computeCloudBlueNoiseTexture.GetFormat(), VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    causticTexture =
        Texture{&core, FilePath::causticTexturePath, VK_FORMAT_R8G8B8A8_SRGB};
    causticTexture.CreateImageView().CreateImageSampler();
    causticTexture.TransitionImageLayout(
        causticTexture.GetImage(), causticTexture.GetFormat(),
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               computeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(core.device, &allocInfo,
                                 computeDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

        VkDescriptorImageInfo computeStorageTextureInfo{};
        computeStorageTextureInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        computeStorageTextureInfo.imageView =
            computeStorageTexture.GetImageView();
        computeStorageTextureInfo.sampler = computeStorageTexture.GetSampler();

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = computeDescriptorSets[i];
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].pImageInfo = &computeStorageTextureInfo;
        descriptorWrites[0].descriptorCount = 1;

        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = uniformBuffers[i].GetBuffer();
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(UniformBufferObject);

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = computeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &uniformBufferInfo;

        // Particles storage buffer
        VkDescriptorBufferInfo particlesStorageBuffer{};
        particlesStorageBuffer.buffer =
            shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT].GetBuffer();
        particlesStorageBuffer.offset = 0;
        particlesStorageBuffer.range = sizeof(Particle) * PARTICLE_COUNT;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = computeDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &particlesStorageBuffer;

        // Noise texture sampler
        VkDescriptorImageInfo noiseTextureInfo{
            computeCloudNoiseTexture.GetSampler(),
            computeCloudNoiseTexture.GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = computeDescriptorSets[i];
        descriptorWrites[3].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[3].pImageInfo = &noiseTextureInfo;
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].descriptorCount = 1;
        // Noise texture sampler
        VkDescriptorImageInfo blueNoiseTextureInfo{
            computeCloudBlueNoiseTexture.GetSampler(),
            computeCloudBlueNoiseTexture.GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = computeDescriptorSets[i];
        descriptorWrites[4].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[4].pImageInfo = &blueNoiseTextureInfo;
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].descriptorCount = 1;

        // caustic texture
        VkDescriptorImageInfo causticTextureInfo{
            causticTexture.GetSampler(), causticTexture.GetImageView(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet = computeDescriptorSets[i];
        descriptorWrites[5].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[5].pImageInfo = &causticTextureInfo;
        descriptorWrites[5].dstBinding = 5;
        descriptorWrites[5].descriptorCount = 1;

        vkUpdateDescriptorSets(core.device, descriptorWrites.size(),
                               descriptorWrites.data(), 0, nullptr);
    }
}
void Application::createGraphicsDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                               graphicsDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    graphicsDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(core.device, &allocInfo,
                                 graphicsDescriptorSets.data()) != VK_SUCCESS) {
        std::cout << vkAllocateDescriptorSets(core.device, &allocInfo,
                                              graphicsDescriptorSets.data())
                  << std::endl;
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo computeStorageTextureInfo{};
        computeStorageTextureInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        computeStorageTextureInfo.imageView =
            computeStorageTexture.GetImageView();
        computeStorageTextureInfo.sampler = computeStorageTexture.GetSampler();


        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = graphicsDescriptorSets[i];
        descriptorWrites[0].descriptorType =
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].pImageInfo = &computeStorageTextureInfo;
        descriptorWrites[0].descriptorCount = 1;

        vkUpdateDescriptorSets(core.device, descriptorWrites.size(),
                               descriptorWrites.data(), 0, nullptr);
    }
}

void Application::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = core.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(core.device, &allocInfo,
                                 commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
    std::cout << "[INFO] Vulkan command buffer created..." << std::endl;
}

void Application::createComputeCommandBuffers()
{
    computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = core.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();

    if (vkAllocateCommandBuffers(core.device, &allocInfo,
                                 computeCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate compute command buffers!");
    }
    std::cout << "[INFO] Vulkan command buffer created..." << std::endl;
}

void Application::recordCommandBuffer(VkCommandBuffer commandBuffer,
                                      uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            graphicsPipelineLayout, 0, 1,
                            &graphicsDescriptorSets[currentFrame], 0, nullptr);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = {0};

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    uiInterface.RecordToCommandBuffer(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Application::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(core.device, &semaphoreInfo, nullptr,
                              &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(core.device, &semaphoreInfo, nullptr,
                              &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(core.device, &fenceInfo, nullptr,
                          &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create graphics synchronization objects for a "
                "frame!");
        }
        if (vkCreateSemaphore(core.device, &semaphoreInfo, nullptr,
                              &computeFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(core.device, &fenceInfo, nullptr,
                          &computeInFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create compute synchronization objects for a "
                "frame!");
        }
    }
}

void Application::recordComputeCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error(
            "failed to begin recording compute command buffer!");
    }

    vkCmdUpdateBuffer(commandBuffer,
                      shaderStorageBuffers[currentFrame].GetBuffer(), 0,
                      sizeof(Particle) * PARTICLE_COUNT, particles.data());

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      core.CurrentPipeline == 0 ? computeFluidPipeline
                                                : computeSmokePipeline);

    vkCmdBindDescriptorSets(
        commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
        core.CurrentPipeline == 0 ? computeFluidPipelineLayout
                                  : computeSmokePipelineLayout,
        0, 1, &computeDescriptorSets[currentFrame], 0, nullptr);

    vkCmdDispatch(commandBuffer, WIDTH / 16 + 1, HEIGHT / 16 + 1, 1);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record compute command buffer!");
    }
}

void Application::drawFrame()
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Compute submission
    vkWaitForFences(core.device, 1, &computeInFlightFences[currentFrame],
                    VK_TRUE, UINT64_MAX);

    updateUniformBuffer(currentFrame);

    vkResetFences(core.device, 1, &computeInFlightFences[currentFrame]);

    vkResetCommandBuffer(computeCommandBuffers[currentFrame],
                         /*VkCommandBufferResetFlagBits*/ 0);
    recordComputeCommandBuffer(computeCommandBuffers[currentFrame]);

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &computeCommandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

    if (vkQueueSubmit(core.computeQueue, 1, &submitInfo,
                      computeInFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit compute command buffer!");
    };

    // Graphics submission
    vkWaitForFences(core.device, 1, &inFlightFences[currentFrame], VK_TRUE,
                    UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        core.device, swapChain, UINT64_MAX,
        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(core.device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame],
                         /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSemaphore waitSemaphores[] = {computeFinishedSemaphores[currentFrame],
                                    imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 2;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

    if (vkQueueSubmit(core.graphicsQueue, 1, &submitInfo,
                      inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(core.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    ++frames;
}

VkShaderModule Application::createShaderModule(
    const std::vector<char>& code) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(core.device, &createInfo, nullptr,
                             &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

VkSurfaceFormatKHR Application::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Application::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        /**
         * https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/performance/swapchain_images#best-practice-summary
         */
        if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::chooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(core.window, &width, &height);

        VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                                   static_cast<uint32_t>(height)};

        actualExtent.width =
            std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                       capabilities.maxImageExtent.width);
        actualExtent.height =
            std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                       capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

std::vector<const char *> Application::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions,
                                         glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    extensions.push_back(
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

    return extensions;
}

void Application::UpdateParticle(std::vector<Particle>& particles)
{
     // TODO: Change to SPH if have more time, particle bouncing as fake effect
    for (auto& particle : particles) {
        particle.position += glm::vec4(glm::vec3(uiInterface.GetWindDirectionFromUIInput()[0],
                      uiInterface.GetWindDirectionFromUIInput()[1],
                      uiInterface.GetWindDirectionFromUIInput()[2]),0) * 0.09f;
        particle.position += glm::vec4(particle.velocity, 0);
        if (particle.position.x >= boxMaxX) {
            particle.position.x = boxMaxX;
        }
        else if (particle.position.x <= boxMinX) {
            particle.position.x = boxMinX;
        }
        if (particle.position.y >= boxMaxY) {
            particle.position.y = boxMaxY;
        }
        else if (particle.position.y <= boxMinY) {
            particle.position.y = boxMinY;
        }
        if (particle.position.z >= boxMaxZ) {
            particle.position.z = boxMaxZ;
        }
        else if (particle.position.z <= boxMinZ)
        {
            particle.position.z = boxMinZ;
        }
    }
}
