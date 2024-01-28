#include "ui.h"

void UserInterface::Init(uint32_t imageCount, VkRenderPass& renderPass)
{
    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    vkCreateDescriptorPool(this->core->device, &pool_info, nullptr, &imguiPool);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(core->window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = core->instance;
    init_info.PhysicalDevice = core->physicalDevice;
    init_info.Device = core->device;
    init_info.Queue = core->graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = imguiPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = imageCount;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&init_info, renderPass);
}

void UserInterface::Render()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Volumetric Rendering Window");
    ImGui::SeparatorText("Team:");
    ImGui::Text("Karl Zakhary, Pascal Neubert, Ao Gao");
    ImGui::Separator();
    
    ImGui::Text("FPS:  %.3f", 1.0 / io.DeltaTime);
    std::string currentSimulation =
        fmt::format("Current simulation: {}", core->CurrentPipeline == 0
                                                  ? "Fluid simulation"
                                                  : "Smoke simulation");
    ImGui::Text(currentSimulation.c_str());
    if (ImGui::Button("Change simulation")){
        fmt::print("Switching to: {}\n", core->CurrentPipeline == 0
                                                  ? "Fluid simulation"
                                                  : "Smoke simulation");
        core->CurrentPipeline = (core->CurrentPipeline + 1) % 2;
    }

    ImGui::SliderFloat3("Sun position", uiSunPosition, -10.0f, 10.0f);
    if (uiSunPosition[1] > 0) uiSunPosition[1] = -0.001; // sun should never go under the ground

    if (ImGui::CollapsingHeader("Movement")) {
        ImGui::BulletText("WSAD: Forward, Backward, Left, Right");
        ImGui::BulletText("Space: up");
        ImGui::BulletText("Ctrl: down");
    }
    ImGui::End();
    // Add UI stuffs here
    ImGui::Render();
}

void UserInterface::RecordToCommandBuffer(VkCommandBuffer commandBuffer)
{
    // ImGui must draw after drawing the command buffer, otherwise it's glitchy
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

void UserInterface::Cleanup()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(core->device, imguiPool, nullptr);
}
