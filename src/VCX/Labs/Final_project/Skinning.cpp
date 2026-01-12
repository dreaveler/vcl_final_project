#include "Labs/Final_project/Skinning.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace VCX::Labs::Final::Skinning {
    std::vector<std::vector<int>> BuildAdjacency(Engine::SurfaceMesh const & mesh) {
        std::vector<std::vector<int>> neighbors(mesh.Positions.size());
        auto add_edge = [&](std::size_t a, std::size_t b) {
            if (a >= neighbors.size() || b >= neighbors.size())
                return;
            neighbors[a].push_back(static_cast<int>(b));
            neighbors[b].push_back(static_cast<int>(a));
        };
        for (std::size_t i = 0; i + 2 < mesh.Indices.size(); i += 3) {
            std::size_t i0 = static_cast<std::size_t>(mesh.Indices[i + 0]);
            std::size_t i1 = static_cast<std::size_t>(mesh.Indices[i + 1]);
            std::size_t i2 = static_cast<std::size_t>(mesh.Indices[i + 2]);
            add_edge(i0, i1);
            add_edge(i1, i2);
            add_edge(i2, i0);
        }
        for (auto & list : neighbors) {
            std::sort(list.begin(), list.end());
            list.erase(std::unique(list.begin(), list.end()), list.end());
        }
        return neighbors;
    } //得到顶点的邻接图

    std::pair<std::vector<int>, std::vector<std::vector<int>>> BuildComponents(
        std::vector<std::vector<int>> const & neighbors) {
        std::vector<int> comp_id(neighbors.size(), -1);
        std::vector<std::vector<int>> components;
        int current = 0;
        for (std::size_t v = 0; v < neighbors.size(); v++) {
            if (comp_id[v] != -1)
                continue;
            components.emplace_back();
            std::vector<int> stack;
            stack.push_back(static_cast<int>(v));
            comp_id[v] = current;
            while (!stack.empty()) {
                int cur = stack.back();
                stack.pop_back();
                components[static_cast<std::size_t>(current)].push_back(cur);
                for (int nb : neighbors[static_cast<std::size_t>(cur)]) {
                    if (comp_id[static_cast<std::size_t>(nb)] != -1)
                        continue;
                    comp_id[static_cast<std::size_t>(nb)] = current;
                    stack.push_back(nb);
                }
            }
            current++;
        }
        return { comp_id, components };
    }  //DFS 由于dancing对应的mesh是n个连通分量组成的 因此遍历并得到连通分量

    float DistanceToSegment(glm::vec3 const & p, glm::vec3 const & a, glm::vec3 const & b, float & t_out) {
        glm::vec3 ab = b - a;
        float ab_len2 = glm::dot(ab, ab);
        if (ab_len2 <= 1e-8f) {
            t_out = 0.0f;
            return glm::length(p - a);
        }
        float t = glm::dot(p - a, ab) / ab_len2;
        t = std::clamp(t, 0.0f, 1.0f);
        t_out = t;
        glm::vec3 proj = a + t * ab;
        return glm::length(p - proj);
    }  //p为网格顶点  a b 为骨骼两端点  t_out为p到ab的投影点在ab中的比例  return p到ab的最短距离

    bool BuildSkinningData(
        Engine::SurfaceMesh const & bindMesh,
        Motion const & motion,
        float skeletonScale,
        Options const & options,
        std::vector<Influence> & weights,
        std::vector<glm::mat4> & invBind) {
        weights.clear();
        invBind.clear();

        if (motion.FrameCount() == 0 || bindMesh.Positions.empty())
            return false;

        auto joints = motion.frames[0].DFSJoints();
        if (joints.empty())
            return false;

        std::vector<glm::vec3> bind_positions;
        bind_positions.resize(joints.size());
        invBind.resize(joints.size());

        for (std::size_t i = 0; i < joints.size(); i++) {
            glm::vec3 pos = joints[i]->get_globaltrans() * skeletonScale;
            glm::quat rot = joints[i]->get_globalrot();
            glm::mat4 bind = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
            invBind[i] = glm::inverse(bind);
            bind_positions[i] = pos;
        }

        auto segments = motion.frames[0].GetSegmentIndices();

        constexpr int kMaxInfluence = 4;
        int max_influences = kMaxInfluence;
        float weight_radius = 0.5f;
        float falloff_power = 2.0f;
        float min_weight = 0.02f;
        std::size_t vcount = bindMesh.Positions.size();
        std::size_t jcount = bind_positions.size();
        weights.resize(vcount);

        auto neighbors = BuildAdjacency(bindMesh);
        auto const component_info = BuildComponents(neighbors);
        auto const & comp_id = component_info.first;
        auto const & components = component_info.second;

        std::vector<glm::vec3> comp_centers(components.size(), glm::vec3(0.0f));
        std::vector<int> comp_counts(components.size(), 0);
        for (std::size_t c = 0; c < components.size(); c++) {
            for (int v : components[c]) {
                comp_centers[c] += bindMesh.Positions[static_cast<std::size_t>(v)];
                comp_counts[c] += 1;
            }
            if (comp_counts[c] > 0)
                comp_centers[c] /= static_cast<float>(comp_counts[c]);
        }

        std::vector<std::vector<unsigned char>> allowed_joints;
        if (options.componentMaxJoints > 0 && !segments.empty()) {
            allowed_joints.assign(components.size(), std::vector<unsigned char>(jcount, 0));
            int cap = std::min(options.componentMaxJoints, static_cast<int>(jcount));
            for (std::size_t c = 0; c < components.size(); c++) {
                std::vector<std::pair<float, std::size_t>> seg_dists;
                seg_dists.reserve(segments.size());
                for (std::size_t s = 0; s < segments.size(); s++) {
                    auto const & seg = segments[s];
                    if (seg.first >= jcount || seg.second >= jcount)
                        continue;
                    float t = 0.0f;
                    float dist = DistanceToSegment(comp_centers[c], bind_positions[seg.first], bind_positions[seg.second], t);
                    seg_dists.emplace_back(dist, s);
                }
                std::sort(seg_dists.begin(), seg_dists.end(),
                          [](auto const & a, auto const & b) { return a.first < b.first; });
                int count = 0;
                for (auto const & item : seg_dists) {
                    auto const & seg = segments[item.second];
                    if (seg.first < jcount && allowed_joints[c][seg.first] == 0) {
                        allowed_joints[c][seg.first] = 1;
                        count++;
                    }
                    if (seg.second < jcount && allowed_joints[c][seg.second] == 0) {
                        allowed_joints[c][seg.second] = 1;
                        count++;
                    }
                    if (count >= cap)
                        break;
                }
                if (count == 0)
                    std::fill(allowed_joints[c].begin(), allowed_joints[c].end(), 1);
            }
        }

        std::vector<std::vector<float>> weight_field(jcount, std::vector<float>(vcount, 0.0f));

        for (std::size_t v = 0; v < vcount; v++) {
            std::vector<float> joint_weights(jcount, 0.0f);
            float nearest_dist = std::numeric_limits<float>::max();
            std::size_t nearest_seg = 0;
            bool found_nearest = false;
            std::vector<unsigned char> const * allowed = nullptr;
            if (!allowed_joints.empty() && v < comp_id.size()) {
                int cid = comp_id[v];
                if (cid >= 0 && static_cast<std::size_t>(cid) < allowed_joints.size())
                    allowed = &allowed_joints[static_cast<std::size_t>(cid)];
            }
            for (auto const & seg : segments) {
                if (seg.first >= jcount || seg.second >= jcount)
                    continue;
                if (allowed && !(*allowed)[seg.first] && !(*allowed)[seg.second])
                    continue;
                float t = 0.0f;
                float dist = DistanceToSegment(bindMesh.Positions[v], bind_positions[seg.first], bind_positions[seg.second], t);
                if (dist < nearest_dist) {
                    nearest_dist = dist;
                    nearest_seg = static_cast<std::size_t>(&seg - &segments[0]);
                    found_nearest = true;
                }
                float w = 0.0f;
                if (weight_radius > 0.0f) {
                    float s = std::max(0.0f, 1.0f - (dist / weight_radius));
                    w = std::pow(s, std::max(0.0f, falloff_power));
                } else {
                    w = 1.0f / (dist + 1e-6f);
                }
                if (w <= 0.0f)
                    continue;
                float w_parent = (1.0f - t) * w;
                float w_child = t * w;
                if (!allowed || (*allowed)[seg.first])
                    joint_weights[seg.first] += w_parent;
                if (!allowed || (*allowed)[seg.second])
                    joint_weights[seg.second] += w_child;
            }

            for (std::size_t j = 0; j < jcount; j++)
                weight_field[j][v] = joint_weights[j];
        }

        if (options.heatIterations > 0 && vcount > 0 && jcount > 0) {
            float lambda = std::clamp(options.heatLambda, 0.0f, 1.0f);
            int iterations = std::max(1, options.heatIterations);
            float anchor_radius = std::max(0.0f, options.heatAnchorRadius);
            std::vector<std::vector<unsigned char>> anchored;
            if (anchor_radius > 0.0f) {
                anchored.assign(jcount, std::vector<unsigned char>(vcount, 0));
                float r2 = anchor_radius * anchor_radius;
                for (std::size_t j = 0; j < jcount; j++) {
                    glm::vec3 jp = bind_positions[j];
                    for (std::size_t v = 0; v < vcount; v++) {
                        glm::vec3 diff = bindMesh.Positions[v] - jp;
                        if (glm::dot(diff, diff) <= r2)
                            anchored[j][v] = 1;
                    }
                }
            }

            std::vector<float> tmp(vcount);
            for (std::size_t j = 0; j < jcount; j++) {
                for (int it = 0; it < iterations; it++) {
                    for (std::size_t v = 0; v < vcount; v++) {
                        if (! anchored.empty() && anchored[j][v]) {
                            tmp[v] = weight_field[j][v];
                            continue;
                        }
                        auto const & nb = neighbors[v];
                        if (nb.empty()) {
                            tmp[v] = weight_field[j][v];
                            continue;
                        }
                        float sum = 0.0f;
                        for (int idx : nb)
                            sum += weight_field[j][static_cast<std::size_t>(idx)];
                        float avg = sum / static_cast<float>(nb.size());
                        tmp[v] = (1.0f - lambda) * weight_field[j][v] + lambda * avg;
                    }
                    weight_field[j].swap(tmp);
                }
            }
        }

        for (std::size_t v = 0; v < vcount; v++) {
            std::vector<unsigned char> const * allowed = nullptr;
            if (!allowed_joints.empty() && v < comp_id.size()) {
                int cid = comp_id[v];
                if (cid >= 0 && static_cast<std::size_t>(cid) < allowed_joints.size())
                    allowed = &allowed_joints[static_cast<std::size_t>(cid)];
            }
            std::array<float, kMaxInfluence> best_w;
            std::array<int, kMaxInfluence> best_idx;
            best_w.fill(0.0f);
            best_idx.fill(-1);

            for (std::size_t j = 0; j < jcount; j++) {
                if (allowed && !(*allowed)[j])
                    continue;
                float w = weight_field[j][v];
                if (w < min_weight)
                    continue;
                if (w <= 0.0f)
                    continue;
                for (int k = 0; k < kMaxInfluence; k++) {
                    if (w > best_w[k]) {
                        for (int s = kMaxInfluence - 1; s > k; s--) {
                            best_w[s] = best_w[s - 1];
                            best_idx[s] = best_idx[s - 1];
                        }
                        best_w[k] = w;
                        best_idx[k] = static_cast<int>(j);
                        break;
                    }
                }
            }

            Influence inf;
            inf.joints = best_idx;
            inf.weights.fill(0.0f);

            float sum = 0.0f;
            for (int k = 0; k < max_influences; k++) {
                if (best_idx[k] < 0) continue;
                inf.weights[k] = best_w[k];
                sum += best_w[k];
            }
            if (sum > 0.0f) {
                for (int k = 0; k < max_influences; k++) {
                    inf.weights[k] /= sum;
                }
            } else {
                float best_dist = std::numeric_limits<float>::max();
                int best_joint = -1;
                for (std::size_t j = 0; j < jcount; j++) {
                    if (allowed && !(*allowed)[j])
                        continue;
                    glm::vec3 diff = bindMesh.Positions[v] - bind_positions[j];
                    float dist = glm::dot(diff, diff);
                    if (dist < best_dist) {
                        best_dist = dist;
                        best_joint = static_cast<int>(j);
                    }
                }
                if (best_joint >= 0) {
                    inf.joints.fill(-1);
                    inf.weights.fill(0.0f);
                    inf.joints[0] = best_joint;
                    inf.weights[0] = 1.0f;
                }
            }

            weights[v] = inf;
        }

        return true;
    }

    bool ApplySkinning(
        Engine::SurfaceMesh const & bindMesh,
        Motion const & motion,
        std::size_t frameIndex,
        float skeletonScale,
        std::vector<Influence> const & weights,
        std::vector<glm::mat4> const & invBind,
        Engine::SurfaceMesh & outMesh) {
        if (weights.empty() || invBind.empty())
            return false;
        if (frameIndex >= motion.FrameCount())
            return false;

        auto joints = motion.frames[frameIndex].DFSJoints();
        if (joints.size() != invBind.size())
            return false;

        std::vector<glm::mat4> joint_mats(joints.size());
        for (std::size_t i = 0; i < joints.size(); i++) {
            glm::vec3 pos = joints[i]->get_globaltrans() * skeletonScale;
            glm::quat rot = joints[i]->get_globalrot();
            joint_mats[i] = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
        }
        outMesh = bindMesh;
        for (std::size_t v = 0; v < bindMesh.Positions.size(); v++) {
            glm::vec4 base = glm::vec4(bindMesh.Positions[v], 1.0f);
            glm::vec4 sum(0.0f);
            for (int k = 0; k < 4; k++) {
                int idx = weights[v].joints[k];
                if (idx < 0) continue;
                float w = weights[v].weights[k];
                sum += w * (joint_mats[idx] * invBind[idx] * base);
            }
            outMesh.Positions[v] = glm::vec3(sum);
        }
        outMesh.Normals = outMesh.ComputeNormals();
        return true;
    }
}
