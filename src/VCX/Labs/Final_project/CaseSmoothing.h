#pragma once

#include "Labs/Final_project/Content.h"
#include "Labs/Final_project/Viewer.h"
#include "Labs/Common/OrbitCameraManager.h"

namespace VCX::Labs::Final {

    class CaseSmoothing : public Common::ICase {
    public:
        CaseSmoothing(Viewer & viewer, std::initializer_list<Assets::ExampleModel> && models);

        virtual std::string_view const GetName() override { return "Mesh Smoothing"; }

        virtual void                     OnSetupPropsUI() override;
        virtual Common::CaseRenderResult OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) override;
        virtual void                     OnProcessInput(ImVec2 const & pos) override;

    private:
        std::vector<Assets::ExampleModel> const _models;

        Viewer                             &  _viewer;
        Engine::Camera                        _camera           { .Eye = glm::vec3(-1, 1, 1) };
        Common::OrbitCameraManager            _cameraManager    { glm::vec3(-1, 1, 1) };
        std::size_t                           _modelIdx         { 0 };
        bool                                  _recompute        { true };
        ModelObject                           _modelObject;
        RenderOptions                         _options;

        char const *                GetModelName(std::size_t const i) const { return Content::ModelNames[std::size_t(_models[i])].c_str(); }
        Engine::SurfaceMesh const & GetModelMesh(std::size_t const i) const { return Content::ModelMeshes[std::size_t(_models[i])]; }
    };
} // namespace VCX::Labs::Final
