
#include <algorithm>
#include <array>
#include <limits>
#include <span>
#include <string>

#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Engine/app.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>
#include <commdlg.h>
#endif

#include "Labs/Final_project/CaseSmoothing.h"
#include "Labs/Common/ImGuiHelper.h"

namespace VCX::Labs::Final {

    static constexpr auto c_Size = std::pair<std::uint32_t, std::uint32_t> { 800U, 600U };

namespace {
#ifdef _WIN32
    bool OpenBVHFileDialog(std::array<char, 260> & path_buffer) {
        OPENFILENAMEA ofn {};
        ofn.lStructSize = sizeof(ofn);
        HWND owner = glfwGetWin32Window(glfwGetCurrentWindow());
        if (! owner) owner = GetActiveWindow();
        ofn.hwndOwner = owner;
        ofn.lpstrFile = path_buffer.data();
        ofn.nMaxFile = static_cast<DWORD>(path_buffer.size());
        ofn.lpstrFilter = "BVH Files\0*.bvh\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
        std::string current(path_buffer.data());
        std::string initial_dir;
        auto pos = current.find_last_of("\\/");
        if (pos != std::string::npos) {
            initial_dir = current.substr(0, pos);
            ofn.lpstrInitialDir = initial_dir.c_str();
        }
        if (ofn.lpstrFile) ofn.lpstrFile[0] = '\0';
        return GetOpenFileNameA(&ofn) == TRUE;
    }
#else
    bool OpenBVHFileDialog(std::array<char, 260> &) {
        return false;
    }
#endif
    std::pair<glm::vec3, glm::vec3> ComputeAABB(std::span<glm::vec3 const> points) {
        glm::vec3 min_v(std::numeric_limits<float>::max());
        glm::vec3 max_v(std::numeric_limits<float>::lowest());
        for (auto const & p : points) {
            min_v = glm::min(min_v, p);
            max_v = glm::max(max_v, p);
        }
        return { min_v, max_v };
    }

    float HeightFromAABB(std::pair<glm::vec3, glm::vec3> const & aabb) {
        return aabb.second.y - aabb.first.y;
    }

    glm::vec3 CenterFromAABB(std::pair<glm::vec3, glm::vec3> const & aabb) {
        return 0.5f * (aabb.first + aabb.second);
    }
} // namespace

    CaseSmoothing::CaseSmoothing(Viewer & viewer, std::initializer_list<Assets::ExampleModel> && models) :
        _models(models),
        _viewer(viewer) {
        _cameraManager.EnablePan       = false;
        _cameraManager.AutoRotateSpeed = 0.f;
        _options.LightDirection = glm::vec3(glm::cos(glm::radians(_options.LightDirScalar)), -1.0f, glm::sin(glm::radians(_options.LightDirScalar)));

        std::string default_path = "data/bvh/cmuconvert-mb2-01-09/01/01_02.bvh";
        std::copy_n(default_path.begin(), std::min(default_path.size(), _bvhPath.size() - 1), _bvhPath.begin());
        _bvhPath[_bvhPath.size() - 1] = '\0';

        ResetModel();
    }

    void CaseSmoothing::OnSetupPropsUI() {
        if (ImGui::BeginCombo("Model", GetModelName(_modelIdx))) {
            for (std::size_t i = 0; i < _models.size(); ++i) {
                bool selected = i == _modelIdx;
                if (ImGui::Selectable(GetModelName(i), selected)) {
                    if (! selected) {
                        _modelIdx  = i;
                        _recompute = true;
                    }
                }
            }
            ImGui::EndCombo();
        }
        Common::ImGuiHelper::SaveImage(_viewer.GetTexture(), _viewer.GetSize(), true);
        ImGui::Spacing();

        ImGui::Text("BVH Path");
        ImGui::SameLine();
        float button_width = ImGui::CalcTextSize("Browse").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(80.0f, avail - button_width - spacing));
        ImGui::InputText("##bvh_path", _bvhPath.data(), _bvhPath.size());
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            OpenBVHFileDialog(_bvhPath);
        }
        if (ImGui::Button("Load BVH")) {
            Motion loaded;
            if (LoadBVHAsMotion(_bvhPath.data(), loaded)) {
                _motion = std::move(loaded);
                _loaded = _motion.FrameCount() > 0;
                _frameIndex = 0;
                _timeAccum = 0.0f;
                _play = false;
                _weightsDirty = true;
            } else {
                _loaded = false;
                _skeletonSegments.clear();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(_play ? "Pause" : "Play")) {
            if (_loaded) _play = ! _play;
        }
        if (_loaded && _motion.FrameCount() > 0) {
            int frame = static_cast<int>(_frameIndex);
            ImGui::SliderInt("Frame", &frame, 0, static_cast<int>(_motion.FrameCount() - 1));
            _frameIndex = static_cast<std::size_t>(frame);
            ImGui::Text("Frames: %zu", _motion.FrameCount());
        } else {
            ImGui::Text("Frames: 0");
        }
        if (ImGui::SliderFloat("Skeleton Scale", &_skeletonScale, 0.001f, 0.1f, "%.3f")) {
            _weightsDirty = true;
        }
        if (ImGui::Checkbox("Auto Align Mesh", &_autoAlign)) {
            _weightsDirty = true;
            UpdateAlignedMesh();
            _modelObject.ReplaceMesh(_bindMesh);
        }
        if (ImGui::SliderInt("Heat Iterations", &_heatIterations, 1, 100)) {
            _weightsDirty = true;
        }
        if (ImGui::SliderFloat("Heat Lambda", &_heatLambda, 0.0f, 1.0f, "%.2f")) {
            _weightsDirty = true;
        }
        if (ImGui::SliderFloat("Heat Anchor Radius", &_heatAnchorRadius, 0.0f, 0.2f, "%.3f")) {
            _weightsDirty = true;
        }
        if (ImGui::SliderInt("Component Max Joints", &_componentMaxJoints, 1, 8)) {
            _weightsDirty = true;
        }
        ImGui::Checkbox("Show Skeleton", &_showSkeleton);
        ImGui::Spacing();

        Viewer::SetupRenderOptionsUI(_options, _cameraManager);
    }

    Common::CaseRenderResult CaseSmoothing::OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) {
        if (_recompute) {
            _recompute = false;
            ResetModel();
        }
        if (_loaded && _weightsDirty) {
            Skinning::Options options;
            options.heatIterations = _heatIterations;
            options.heatLambda = _heatLambda;
            options.heatAnchorRadius = _heatAnchorRadius;
            options.componentMaxJoints = _componentMaxJoints;
            UpdateAlignedMesh();
            _weightsDirty = !Skinning::BuildSkinningData(_bindMesh, _motion, _skeletonScale, options, _weights, _invBind);
            _lastFrameIndex = static_cast<std::size_t>(-1);
        }
        if (_loaded && _motion.FrameCount() > 0) {
            if (_play && _motion.frame_time > 0.0f) {
                _timeAccum += ImGui::GetIO().DeltaTime;
                while (_timeAccum >= _motion.frame_time) {
                    _timeAccum -= _motion.frame_time;
                    _frameIndex = (_frameIndex + 1) % _motion.FrameCount();
                }
            }
            if (_frameIndex != _lastFrameIndex) {
                if (Skinning::ApplySkinning(_bindMesh, _motion, _frameIndex, _skeletonScale, _weights, _invBind, _skinnedMesh))
                    _modelObject.ReplaceMesh(_skinnedMesh);
                _skeletonSegments = _motion.frames[_frameIndex].GetSegments();
                for (auto & p : _skeletonSegments) p *= _skeletonScale;
                _lastFrameIndex = _frameIndex;
            }
        }
        std::span<glm::vec3 const> skeleton_lines;
        if (_showSkeleton && ! _skeletonSegments.empty())
            skeleton_lines = std::span<glm::vec3 const>(_skeletonSegments);
        return _viewer.Render(_options, _modelObject, _camera, _cameraManager, desiredSize, skeleton_lines);
    }

    void CaseSmoothing::OnProcessInput(ImVec2 const & pos) {
        _cameraManager.ProcessInput(_camera, pos);
    }

    void CaseSmoothing::ResetModel() {
        _sourceMesh = GetModelMesh(_modelIdx);
        UpdateAlignedMesh();
        _skinnedMesh = _bindMesh;
        _modelObject.ReplaceMesh(_bindMesh);
        _skeletonSegments.clear();
        _weightsDirty = true;
        _frameIndex = 0;
        _lastFrameIndex = static_cast<std::size_t>(-1);
    }

    void CaseSmoothing::UpdateAlignedMesh() {
        _bindMesh = _sourceMesh;
        if (! _autoAlign || ! _loaded || _motion.FrameCount() == 0 || _bindMesh.Positions.empty())
            return;

        auto joints = _motion.frames[0].DFSJoints();
        if (joints.empty())
            return;

        std::vector<glm::vec3> joint_positions;
        joint_positions.reserve(joints.size());
        for (auto const & j : joints)
            joint_positions.push_back(j->get_globaltrans() * _skeletonScale);

        auto mesh_aabb = ComputeAABB(_bindMesh.Positions);
        auto skel_aabb = ComputeAABB(joint_positions);

        float mesh_height = HeightFromAABB(mesh_aabb);
        float skel_height = HeightFromAABB(skel_aabb);
        if (mesh_height <= 1e-6f || skel_height <= 1e-6f)
            return;

        float scale = skel_height / mesh_height;
        glm::vec3 mesh_center = CenterFromAABB(mesh_aabb);
        glm::vec3 root_pos = joints[0]->get_globaltrans() * _skeletonScale;

        for (auto & p : _bindMesh.Positions) {
            p = (p - mesh_center) * scale + root_pos;
        }
    }

} // namespace VCX::Labs::Final
