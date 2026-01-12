#include <functional>
#include <memory>
#include <unordered_map>
#include "HumanDS.h"
#include <glm/glm.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

namespace VCX::Labs::Final{
    using JointPtr = std::shared_ptr<Joint>;
    Joint::Joint(bool _isleaf):isleaf(_isleaf),name("END"){}
    Joint::Joint(std::string _name):name(_name){}
    void Joint::set_rotation(const glm::vec3& rot){   //杩欓噷宸茬粡榛樿杈撳叆鐨剅ot鏄鎷夎锛歑YZ鐨勯『搴忎簡 涔熷氨鏄璇诲叆bvh鏃跺凡缁忛『搴忕煫姝?        euler_rotation = rot;
        rotation = glm::quat(euler_rotation);
    }
    void Joint::set_offset(const glm::vec3& off){
        offset = off;
    }
    void Joint::set_father(const JointPtr& fa){
        father = fa;
    }
    void Joint::add_children(const JointPtr& child){
        children.push_back(child);
    }
    void Joint::update_global(){
        if(father == NULL) return;
        global_rot = father->global_rot * rotation;
        global_trans = father->global_trans + glm::rotate(father->global_rot,offset);
    }

    glm::vec3 Joint::get_globaltrans() const{
        return global_trans;
    }

    glm::quat Joint::get_globalrot() const{
        return global_rot;
    }

    JointPtr HumanDS::CreateJoint(const std::string & name, bool is_leaf) {
        if (is_leaf) return std::make_shared<Joint>(true);
        return std::make_shared<Joint>(name);
    }

    void HumanDS::SetRoot(const JointPtr & r) {
        root = r;
    }

    void HumanDS::AttachChild(const JointPtr & parent, const JointPtr & child) {
        if (! parent || ! child) return;
        child->set_father(parent);
        parent->add_children(child);
    }

    void HumanDS::SetJointOffset(const JointPtr & joint, const glm::vec3 & off) {
        if (! joint) return;
        joint->set_offset(off);
    }

    void HumanDS::SetJointRotationQuat(const JointPtr & joint, const glm::quat & rot) {
        if (! joint) return;
        joint->rotation = rot;
        joint->euler_rotation = glm::eulerAngles(rot);
    }

    glm::vec3 HumanDS::GetJointOffset(const JointPtr & joint) const {
        if (! joint) return glm::vec3(0.0f);
        return joint->offset;
    }

    std::vector<JointPtr> HumanDS::DFSJoints() const {
        std::vector<JointPtr> res;
        if (! root) return res;
        std::function<void(const JointPtr &)> walk = [&](const JointPtr & j) {
            if (! j) return;
            res.push_back(j);
            for (auto const & child : j->children) walk(child);
        };
        walk(root);
        return res;
    }

    std::vector<glm::vec3> HumanDS::GetSegments() const {
        std::vector<glm::vec3> segments;
        auto joints = DFSJoints();
        for (auto const & j : joints) {
            for (auto const & child : j->children) {
                segments.push_back(j->global_trans);
                segments.push_back(child->global_trans);
            }
        }
        return segments;
    }

    std::vector<std::pair<std::size_t, std::size_t>> HumanDS::GetSegmentIndices() const {
        std::vector<std::pair<std::size_t, std::size_t>> segments;
        auto joints = DFSJoints();
        std::unordered_map<Joint*, std::size_t> index_of;
        index_of.reserve(joints.size());
        for (std::size_t i = 0; i < joints.size(); i++) {
            index_of[joints[i].get()] = i;
        }
        for (auto const & j : joints) {
            auto it = index_of.find(j.get());
            if (it == index_of.end()) continue;
            std::size_t parent_idx = it->second;
            for (auto const & child : j->children) {
                auto child_it = index_of.find(child.get());
                if (child_it != index_of.end())
                    segments.emplace_back(parent_idx, child_it->second);
            }
        }
        return segments;
    }

    void HumanDS::UpdateGlobalRecursive(const JointPtr & joint) {
        if (! joint) return;
        joint->update_global();
        for (auto const & child : joint->children) UpdateGlobalRecursive(child);
    }

    void HumanDS::UpdateGlobal() {
        if (! root) return;
        root->global_rot = root->rotation;
        root->global_trans = root->offset;
        for (auto const & child : root->children) UpdateGlobalRecursive(child);
    }

    JointPtr HumanDS::CloneRecursive(const JointPtr & joint, const JointPtr & parent) {
        if (! joint) return nullptr;
        auto clone = CreateJoint(joint->name, joint->isleaf);
        clone->set_offset(joint->offset);
        clone->set_rotation(joint->euler_rotation);
        clone->global_trans = joint->global_trans;
        clone->global_rot = joint->global_rot;
        if (parent) AttachChild(parent, clone);
        for (auto const & child : joint->children) {
            CloneRecursive(child, clone);
        }
        return clone;
    }

    HumanDS HumanDS::Clone() const {
        HumanDS copy;
        if (! root) return copy;
        auto new_root = copy.CloneRecursive(root, nullptr);
        copy.SetRoot(new_root);
        return copy;
    }

    std::vector<glm::vec3> Motion::GetJointPositions(std::size_t frame_idx) const {
        if (frame_idx >= frames.size()) return {};
        std::vector<glm::vec3> out;
        auto joints = frames[frame_idx].DFSJoints();
        out.reserve(joints.size());
        for (auto const & j : joints) out.push_back(j->get_globaltrans());
        return out;
    }
}
