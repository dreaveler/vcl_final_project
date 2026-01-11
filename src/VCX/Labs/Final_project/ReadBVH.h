#pragma once
#ifndef READBVH_H
#define READBVH_H
#include <cstddef>
#include <string>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
#include "HumanDS.h"
namespace VCX::Labs::Final{
    enum class BVHChannelType {
        Xposition,
        Yposition,
        Zposition,
        Xrotation,
        Yrotation,
        Zrotation,
    };

    struct BVHChannel {
        int joint_index = -1;
        BVHChannelType type;
    };

    struct BVHClip {
        std::size_t frame_count = 0;
        float frame_time = 0.0f;
        std::vector<BVHChannel> channels;
        std::vector<float> frames;
    };

    bool LoadBVH(const std::string & path, HumanDS & human, BVHClip & clip);
    bool LoadBVHAsMotion(const std::string & path, Motion & out);

}

#endif
