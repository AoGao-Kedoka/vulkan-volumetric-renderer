#pragma once

#include <string>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

struct FilePath {
    inline const static std::string computeFluidShaderPath{
        "./shaders/volumetric_comp.spv"};
    inline const static std::string computeSmokeShaderPath{
        "./shaders/smoke_comp.spv"};
    inline const static std::string vertexShaderPath{
        "./shaders/volumetric_vert.spv"};
    inline const static std::string fragmentShaderPath{
        "./shaders/volumetric_frag.spv"};
    inline const static std::string causticTexturePath{
        "./textures/caustic.jpg"};
    inline const static std::string computeCloudNoiseTexturePath{
        "./textures/noise.png"};
    inline const static std::string computeCloudBlueNoiseTexturePath{
        "./textures/blue_noise.png"};
};

struct UniformBufferObject {
    float deltaTime = 1.0f;
    float totalTime = 0.0f;
    alignas(16) glm::vec3 sunPosition;
    alignas(16) glm::vec3 cameraPosition;

    alignas(16) glm::vec3 windDirection;
    uint32_t frame = 0;
};

struct Particle {
    alignas(16) glm::vec4 position;  // position.w -> scale of particle;
    alignas(16) glm::vec3 velocity;
    alignas(16) glm::vec4 color;
};

