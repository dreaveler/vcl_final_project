
#include <algorithm>
#include <array>

#include "Labs/Final_project/CaseSmoothing.h"
#include "Labs/Common/ImGuiHelper.h"

namespace VCX::Labs::Final {

    static constexpr auto c_Size = std::pair<std::uint32_t, std::uint32_t> { 800U, 600U };

    CaseSmoothing::CaseSmoothing(Viewer & viewer, std::initializer_list<Assets::ExampleModel> && models) :
        _models(models),
        _viewer(viewer) {
        _cameraManager.EnablePan       = false;
        _cameraManager.AutoRotateSpeed = 0.f;
        _options.LightDirection = glm::vec3(glm::cos(glm::radians(_options.LightDirScalar)), -1.0f, glm::sin(glm::radians(_options.LightDirScalar)));
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

        Viewer::SetupRenderOptionsUI(_options, _cameraManager);
    }

    Common::CaseRenderResult CaseSmoothing::OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) {
        if (_recompute) {
            _recompute = false;
            _modelObject.ReplaceMesh(GetModelMesh(_modelIdx));
        }
        return _viewer.Render(_options, _modelObject, _camera, _cameraManager, desiredSize);
    }

    void CaseSmoothing::OnProcessInput(ImVec2 const & pos) {
        _cameraManager.ProcessInput(_camera, pos);
    }
} // namespace VCX::Labs::Final
