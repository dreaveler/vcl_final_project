#include "ReadBVH.h"

#include <fstream>
#include <string>
#include <vector>

namespace VCX::Labs::Animation {
namespace {

    BVHChannelType ParseChannelType(const std::string & token) {
        if (token == "Xposition") return BVHChannelType::Xposition;
        if (token == "Yposition") return BVHChannelType::Yposition;
        if (token == "Zposition") return BVHChannelType::Zposition;
        if (token == "Xrotation") return BVHChannelType::Xrotation;
        if (token == "Yrotation") return BVHChannelType::Yrotation;
        return BVHChannelType::Zrotation;
    }

    void ReadFloat3(std::istream & in, glm::vec3 & out) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        in >> x >> y >> z;
        out = glm::vec3(x, y, z);
    }

    int ParseEndSite(std::istream & in,
                     HumanDS & human,
                     const JointPtr & parent,
                     std::vector<JointPtr> & joints) {
        std::string token;
        in >> token; // "{"
        auto joint = human.CreateJoint("EndSite", true);
        if (parent) human.AttachChild(parent, joint);

        in >> token; // "OFFSET"
        glm::vec3 offset(0.0f);
        ReadFloat3(in, offset);
        human.SetJointOffset(joint, offset);

        in >> token; // "}"
        int index = static_cast<int>(joints.size());
        joints.push_back(joint);
        return index;
    }

    int ParseJoint(std::istream & in,
                   HumanDS & human,
                   const JointPtr & parent,
                   const std::string & name,
                   bool is_root,
                   std::vector<BVHChannel> & channels,
                   std::vector<JointPtr> & joints) {
        std::string token;
        in >> token; // "{"

        auto joint = human.CreateJoint(name);
        if (is_root) human.SetRoot(joint);
        if (parent) human.AttachChild(parent, joint);

        int index = static_cast<int>(joints.size());
        joints.push_back(joint);

        while (in >> token) {
            if (token == "OFFSET") {
                glm::vec3 offset(0.0f);
                ReadFloat3(in, offset);
                human.SetJointOffset(joint, offset);
            } else if (token == "CHANNELS") {
                int count = 0;
                in >> count;
                for (int i = 0; i < count; i++) {
                    std::string chan;
                    in >> chan;
                    channels.push_back(BVHChannel { index, ParseChannelType(chan) });
                }
            } else if (token == "JOINT") {
                std::string child_name;
                in >> child_name;
                ParseJoint(in, human, joint, child_name, false, channels, joints);
            } else if (token == "End") {
                std::string site;
                in >> site; // "Site"
                ParseEndSite(in, human, joint, joints);
            } else if (token == "}") {
                return index;
            }
        }
        return index;
    }

} // namespace

bool LoadBVH(const std::string & path, HumanDS & human, BVHClip & clip) {
    std::ifstream file(path);
    std::string token;

    file >> token; // HIERARCHY
    file >> token; // ROOT
    std::string root_name;
    file >> root_name;

    std::vector<JointPtr> joints;
    ParseJoint(file, human, nullptr, root_name, true, clip.channels, joints);

    file >> token; // MOTION
    file >> token; // Frames:
    file >> clip.frame_count;
    file >> token; // Frame
    file >> token; // Time:
    file >> clip.frame_time;

    const std::size_t channel_count = clip.channels.size();
    const std::size_t total_values = clip.frame_count * channel_count;
    clip.frames.resize(total_values, 0.0f);
    for (std::size_t i = 0; i < total_values; i++) {
        file >> clip.frames[i];
    }

    return true;
}

bool LoadBVHAsMotion(const std::string & path, Motion & out) {
    HumanDS base;
    BVHClip clip;
    if (! LoadBVH(path, base, clip)) return false;

    auto base_joints = base.DFSJoints();
    const std::size_t joint_count = base_joints.size();
    const std::size_t channel_count = clip.channels.size();

    out.frames.clear();
    out.frames.reserve(clip.frame_count);
    out.frame_time = clip.frame_time;

    for (std::size_t f = 0; f < clip.frame_count; f++) {
        HumanDS frame = base.Clone();
        auto frame_joints = frame.DFSJoints();

        std::vector<glm::vec3> pos_delta(joint_count, glm::vec3(0.0f));
        std::vector<glm::vec3> rot_euler(joint_count, glm::vec3(0.0f));

        for (std::size_t c = 0; c < channel_count; c++) {
            float v = clip.frames[f * channel_count + c];
            const auto & ch = clip.channels[c];
            int idx = ch.joint_index;
            if (idx < 0 || static_cast<std::size_t>(idx) >= joint_count) continue;
            if (ch.type == BVHChannelType::Xrotation) rot_euler[idx].x = v;
            if (ch.type == BVHChannelType::Yrotation) rot_euler[idx].y = v;
            if (ch.type == BVHChannelType::Zrotation) rot_euler[idx].z = v;
            if (ch.type == BVHChannelType::Xposition) pos_delta[idx].x = v;
            if (ch.type == BVHChannelType::Yposition) pos_delta[idx].y = v;
            if (ch.type == BVHChannelType::Zposition) pos_delta[idx].z = v;
        }

        for (std::size_t i = 0; i < joint_count; i++) {
            auto & joint = frame_joints[i];
            glm::vec3 base_offset = base.GetJointOffset(base_joints[i]);
            if (pos_delta[i] != glm::vec3(0.0f)) {
                frame.SetJointOffset(joint, base_offset + pos_delta[i]);
            }
            if (rot_euler[i] != glm::vec3(0.0f)) {
                frame.SetJointRotation(joint, glm::radians(rot_euler[i]));
            }
        }

        frame.UpdateGlobal();
        out.frames.push_back(frame);
    }

    return true;
}

} // namespace VCX::Labs::Animation
