#pragma once

#include <string>

struct FilePath {
    inline const static std::string computeShaderPath{
        "./shaders/volumetric_comp.spv"};
    inline const static std::string smokeShaderPath{"./shaders/smoke_comp.spv"};
    inline const static std::string vertexShaderPath{
        "./shaders/volumetric_vert.spv"};
    inline const static std::string fragmentShaderPath{
        "./shaders/volumetric_frag.spv"};
    inline const static std::string causticTexturePath{
        "./textures/caustic.jpg"};
    inline const static std::string computeCloudNoiseTexturePath{
        "./textures/noise.png"};
};
