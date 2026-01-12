#pragma once
#ifndef HUMANDS_H
#define HUMANDS_H
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>
namespace VCX::Labs::Final{
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
        glm::quat get_globalrot() const;
    };
    class HumanDS{
        JointPtr root;
        void UpdateGlobalRecursive(const JointPtr & joint);   //递归更新
        JointPtr CloneRecursive(const JointPtr & joint, const JointPtr & parent);   //递归复制
    public:
        JointPtr CreateJoint(const std::string & name, bool is_leaf = false);
        void SetRoot(const JointPtr & r);
        void AttachChild(const JointPtr & parent, const JointPtr & child);
        void SetJointOffset(const JointPtr & joint, const glm::vec3 & off);
        void SetJointRotationQuat(const JointPtr & joint, const glm::quat & rot);
        glm::vec3 GetJointOffset(const JointPtr & joint) const;
        std::vector<JointPtr> DFSJoints() const;
        std::vector<glm::vec3> GetSegments() const;  //得到骨骼的分离形式
        std::vector<std::pair<std::size_t, std::size_t>> GetSegmentIndices() const;   //得到骨骼两端序号在DFS下的集合
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
