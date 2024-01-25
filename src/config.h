#pragma once

#include <string>

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
