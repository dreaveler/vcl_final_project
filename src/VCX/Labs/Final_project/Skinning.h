#pragma once

#include <array>
#include <vector>

#include <glm/glm.hpp>

#include "Engine/SurfaceMesh.h"
#include "ReadBVH.h"

namespace VCX::Labs::Final::Skinning {
    struct Options {
        int   heatIterations = 20;
        float heatLambda = 0.6f;
        float heatAnchorRadius = 0.05f;
        int   componentMaxJoints = 3;
    };

    struct Influence {
        std::array<int, 4>   joints;
        std::array<float, 4> weights;
    };

    bool BuildSkinningData(
        Engine::SurfaceMesh const & bindMesh,
        Motion const & motion,
        float skeletonScale,
        Options const & options,
        std::vector<Influence> & weights,
        std::vector<glm::mat4> & invBind);

    bool ApplySkinning(
        Engine::SurfaceMesh const & bindMesh,
        Motion const & motion,
        std::size_t frameIndex,
        float skeletonScale,
        std::vector<Influence> const & weights,
        std::vector<glm::mat4> const & invBind,
        Engine::SurfaceMesh & outMesh);
}
