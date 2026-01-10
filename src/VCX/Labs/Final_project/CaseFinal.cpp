#include <algorithm>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#endif
#include "Engine/app.h"
#include "Labs/Final_project/CaseFinal.h"
#include "Labs/Common/ImGuiHelper.h"

namespace VCX::Labs::Animation {
namespace {
#ifdef _WIN32
    bool OpenBVHFileDialog(std::array<char, 260> & path_buffer) {
        OPENFILENAMEA ofn {};
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = path_buffer.data();
        ofn.nMaxFile = static_cast<DWORD>(path_buffer.size());
        ofn.lpstrFilter = "BVH Files\0*.bvh\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        return GetOpenFileNameA(&ofn) == TRUE;
    }
#else
    bool OpenBVHFileDialog(std::array<char, 260> &) {
        return false;
    }
#endif
} // namespace

    BackGroundRender::BackGroundRender():
        LineItem(Engine::GL::VertexLayout().Add<glm::vec3>("position", Engine::GL::DrawFrequency::Stream, 0), Engine::GL::PrimitiveType::Lines),
        PointItem(Engine::GL::VertexLayout().Add<glm::vec3>("position", Engine::GL::DrawFrequency::Stream, 0), Engine::GL::PrimitiveType::Points) {
        const std::uint32_t index[] = { 0, 1, 0, 2, 0, 3 };
        float pos[12] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
        LineItem.UpdateElementBuffer(index);
        LineItem.UpdateVertexBuffer("position", std::span<std::byte>(reinterpret_cast<std::byte *>(pos), reinterpret_cast<std::byte *>(pos + 12)));
    }

    void BackGroundRender::UpdatePoints(const std::vector<glm::vec3> & points) {
        if (points.empty()) {
            PointItem.UpdateVertexBuffer("position", std::span<std::byte>());
            return;
        }
        PointItem.UpdateVertexBuffer("position", Engine::make_span_bytes<glm::vec3>(points));
    }

    void BackGroundRender::render(Engine::GL::UniqueProgram & program, bool draw_axis) {
        if (draw_axis) {
            program.GetUniforms().SetByName("u_Color", glm::vec3(0.0f, 0.8f, 0.0f));
            LineItem.Draw({ program.Use() });
        }
        program.GetUniforms().SetByName("u_Color", glm::vec3(1.0f, 0.6f, 0.0f));
        PointItem.Draw({ program.Use() });
    }

    BoxRenderer::BoxRenderer():
        CenterPosition(0, 0, 0),
        MainAxis(0, 1, 0),
        BoxItem(Engine::GL::VertexLayout().Add<glm::vec3>("position", Engine::GL::DrawFrequency::Stream, 0), Engine::GL::PrimitiveType::Triangles),
        LineItem(Engine::GL::VertexLayout().Add<glm::vec3>("position", Engine::GL::DrawFrequency::Stream, 0), Engine::GL::PrimitiveType::Lines) {
        //     3-----2
        //    /|    /|
        //   0 --- 1 |
        //   | 7 - | 6
        //   |/    |/
        //   4 --- 5
        VertsPosition.resize(8);
        const std::vector<std::uint32_t> line_index = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7}; // line index
        LineItem.UpdateElementBuffer(line_index);

        const std::vector<std::uint32_t> tri_index = { 0, 1, 2, 0, 2, 3, 1, 4, 0, 1, 4, 5, 1, 6, 5, 1, 2, 6, 2, 3, 7, 2, 6, 7, 0, 3, 7, 0, 4, 7, 4, 5, 6, 4, 6, 7};
        BoxItem.UpdateElementBuffer(tri_index);
    }

    void BoxRenderer::render(Engine::GL::UniqueProgram & program)
    {
        auto span_bytes = Engine::make_span_bytes<glm::vec3>(VertsPosition);
        
        program.GetUniforms().SetByName("u_Color", glm::vec3(121.0f / 255, 207.0f / 255, 171.0f / 255));
        BoxItem.UpdateVertexBuffer("position", span_bytes);
        BoxItem.Draw({ program.Use() });

        program.GetUniforms().SetByName("u_Color", glm::vec3(1.0f, 1.0f, 1.0f));
        LineItem.UpdateVertexBuffer("position", span_bytes);
        LineItem.Draw({ program.Use() });
    }

    void BoxRenderer::calc_vert_position() {
        glm::vec3 new_y = glm::normalize(MainAxis);
        glm::quat quat = glm::rotation(glm::vec3(0, 1, 0), new_y);
        glm::vec3 new_x = quat * glm::vec3(0.5f * width, 0.0f, 0.0f);
        glm::vec3 new_z = quat * glm::vec3(0.0f, 0.0f, 0.5f * width);
        const glm::vec3 & c = CenterPosition;
        new_y *= 0.5 * length;
        VertsPosition[0] = c - new_x + new_y + new_z;
        VertsPosition[1] = c + new_x + new_y + new_z;
        VertsPosition[2] = c + new_x + new_y - new_z;
        VertsPosition[3] = c - new_x + new_y - new_z;
        VertsPosition[4] = c - new_x - new_y + new_z;
        VertsPosition[5] = c + new_x - new_y + new_z;
        VertsPosition[6] = c + new_x - new_y - new_z;
        VertsPosition[7] = c - new_x - new_y - new_z;
    }

    CaseFinal::CaseFinal():
        BackGround(),
        _program(
            Engine::GL::UniqueProgram({ Engine::GL::SharedShader("assets/shaders/flat.vert"),
                                        Engine::GL::SharedShader("assets/shaders/flat.frag") }))
    {
        _cameraManager.AutoRotate = false;
        _cameraManager.Save(_camera);
        std::string default_path = "data/bvh/cmuconvert-mb2-01-09/01/01_01.bvh";
        std::copy_n(default_path.begin(), std::min(default_path.size(), _pathBuffer.size() - 1), _pathBuffer.begin());
        
        ResetSystem();
    }

    void CaseFinal::BuildBySegments(const std::vector<glm::vec3> & segment_points) {
        if (segment_points.size() < 2) {
            arms.clear();
            return;
        }
        std::size_t required = segment_points.size() / 2;
        while (arms.size() < required) {
            arms.emplace_back();
        }
        while (arms.size() > required) {
            arms.pop_back();
        }
        for (std::size_t i = 0; i < required; i++) {
            auto & a = segment_points[i * 2];
            auto & b = segment_points[i * 2 + 1];
            auto MainAxis = b - a;
            float len = glm::l2Norm(MainAxis);
            if (len <= 1e-6f) continue;
            arms[i].length = len;
            arms[i].width = 0.05f * _scale;
            arms[i].MainAxis = MainAxis / len;
            arms[i].CenterPosition = 0.5f * (a + b);
        }
    }

    void CaseFinal::OnSetupPropsUI() {
        ImGui::Text("BVH Path");
        ImGui::SameLine();
        float button_width = ImGui::CalcTextSize("Browse").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(std::max(80.0f, avail - button_width - spacing));
        ImGui::InputText("##bvh_path", _pathBuffer.data(), _pathBuffer.size());
        ImGui::SameLine();
        if (ImGui::Button("Browse")) {
            OpenBVHFileDialog(_pathBuffer);
        }
        if (ImGui::Button("Load")) {
            Motion loaded;
            if (LoadBVHAsMotion(_pathBuffer.data(), loaded)) {
                _motion = std::move(loaded);
                _loaded = _motion.FrameCount() > 0;
                _frameIndex = 0;
                _timeAccum = 0.0f;
                _play = false;
                if (_loaded) ImGui::Text("加载成功");
            } else {
                _loaded = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(_play ? "Pause" : "Play")) {
            if (_loaded) _play = ! _play;
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) ResetSystem();

        ImGui::SliderFloat("Scale", &_scale, 0.001f, 0.1f, "%.3f");
        ImGui::Checkbox("Show Axis", &_showAxis);
        if (_loaded && _motion.FrameCount() > 0) {
            int frame = static_cast<int>(_frameIndex);
            ImGui::SliderInt("Frame", &frame, 0, static_cast<int>(_motion.FrameCount() - 1));
            _frameIndex = static_cast<std::size_t>(frame);
            ImGui::Text("Frames: %zu", _motion.FrameCount());
        } else {
            ImGui::Text("Frames: 0");
        }
    }

    Common::CaseRenderResult CaseFinal::OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) {
        _frame.Resize(desiredSize);

        _cameraManager.Update(_camera);
        _program.GetUniforms().SetByName("u_Projection", _camera.GetProjectionMatrix((float(desiredSize.first) / desiredSize.second)));
        _program.GetUniforms().SetByName("u_View", _camera.GetViewMatrix());

        gl_using(_frame);
        glEnable(GL_LINE_SMOOTH);
        glLineWidth(0.5f);
        glPointSize(4.f);

        if (_loaded && _motion.FrameCount() > 0) {
            if (_play) {
                _timeAccum += Engine::GetDeltaTime();
                float frame_step = (_motion.frame_time > 0.0f) ? _motion.frame_time : (1.0f / 30.0f);
                while (_timeAccum >= frame_step) {
                    _frameIndex = (_frameIndex + 1) % _motion.FrameCount();
                    _timeAccum -= frame_step;
                }
            }
            auto joint_pos = _motion.GetJointPositions(_frameIndex);
            auto segments = _motion.frames[_frameIndex].GetSegments();
            for (auto & p : joint_pos) p *= _scale;
            for (auto & p : segments) p *= _scale;
            BuildBySegments(segments);
            BackGround.UpdatePoints(joint_pos);
        } else {
            BackGround.UpdatePoints({});
        }
        
        for (auto & arm : arms) {
            arm.calc_vert_position();
            arm.render(_program);
        }
        BackGround.render(_program, _showAxis);

        glLineWidth(1.f);
        glPointSize(1.f);
        glDisable(GL_LINE_SMOOTH);

        return Common::CaseRenderResult {
            .Fixed     = false,
            .Flipped   = true,
            .Image     = _frame.GetColorAttachment(),
            .ImageSize = desiredSize,
        };
    }

    void CaseFinal::OnProcessInput(ImVec2 const & pos) {
        _cameraManager.ProcessInput(_camera, pos);
    }

    void CaseFinal::ResetSystem() {
        _frameIndex = 0;
        _timeAccum = 0.0f;
        _play = false;
    }
} // namespace VCX::Labs::Animation
