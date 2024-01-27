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
};

struct UniformBufferObject {
    float deltaTime = 1.0f;
    float totalTime = 0.0f;
    alignas(16) glm::vec3 sunPosition = glm::vec3(0, -5, 0);
    alignas(16) glm::vec3 cameraPosition = glm::vec3(0, 0, 5.0);
};

struct Particle {
    glm::vec4 position;  // position.w -> scale of particle;
    glm::vec3 velocity;
    glm::vec4 color;
};

