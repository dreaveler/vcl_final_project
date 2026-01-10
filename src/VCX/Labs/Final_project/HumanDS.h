#pragma once
#ifndef HUMANDS_H
#define HUMANDS_H
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
namespace VCX::Labs::Animation{
    class Joint;
    using JointPtr = std::shared_ptr<Joint>;
    class Joint{
    protected:
        friend class HumanDS;
        glm::vec3 euler_rotation;
        glm::quat rotation;
        glm::vec3 offset;
        std::string name;
        JointPtr father;
        glm::vec3 global_trans;
        glm::quat global_rot;
        std::vector<JointPtr>children;
        bool isleaf{false};
        void set_rotation(const glm::vec3&);
        void set_offset(const glm::vec3&);
        void set_father(const JointPtr&);
        void add_children(const JointPtr&);
    public:
        Joint() = default;
        Joint(bool);
        Joint(std::string);
        void update_global();
        glm::vec3 get_globaltrans() const;
    };
    class HumanDS{
        JointPtr root;
        void UpdateGlobalRecursive(const JointPtr & joint);
        JointPtr CloneRecursive(const JointPtr & joint, const JointPtr & parent);
    public:
        JointPtr CreateJoint(const std::string & name, bool is_leaf = false);
        void SetRoot(const JointPtr & r);
        JointPtr GetRoot() const;
        void AttachChild(const JointPtr & parent, const JointPtr & child);
        void SetJointOffset(const JointPtr & joint, const glm::vec3 & off);
        void SetJointRotation(const JointPtr & joint, const glm::vec3 & rot);
        void SetJointGlobal(const JointPtr & joint, const glm::vec3 & trans, const glm::quat & rot);
        glm::vec3 GetJointOffset(const JointPtr & joint) const;
        std::vector<JointPtr> DFSJoints() const;
        std::vector<glm::vec3> GetSegments() const;
        void UpdateGlobal();
        HumanDS Clone() const;
    };

    class Motion{
    public:
        std::vector<HumanDS> frames;
        float frame_time = 0.0f;
        std::size_t FrameCount() const { return frames.size(); }
        std::vector<glm::vec3> GetJointPositions(std::size_t frame_idx) const;
    };
}

#endif
