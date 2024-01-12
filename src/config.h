#pragma once

struct FilePath {
    inline const static std::string computeShaderPath{ "./shaders/particles_comp.spv" };
    inline const static std::string vertexShaderPath{ "./shaders/particles_vert.spv" };
    inline const static std::string fragmentShaderPath{ "./shaders/particles_frag.spv" };
    inline const static std::string causticTexturePath{"./textures/proceduralCaustics.png"};
};
