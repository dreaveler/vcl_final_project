#pragma once

#include <array>
#include <vector>

#include "ReadBVH.h"
#include "Labs/Final_project/Skinning.h"
#include "Labs/Final_project/Content.h"
#include "Labs/Final_project/Viewer.h"
#include "Labs/Common/OrbitCameraManager.h"
#include "Engine/SurfaceMesh.h"

namespace VCX::Labs::Final {

    class CaseSkinning : public Common::ICase {
    public:
        CaseSkinning(Viewer & viewer, std::initializer_list<Assets::ExampleModel> && models);

        virtual std::string_view const GetName() override { return "Final Project:Skinning"; }

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

        Motion                                _motion;
        bool                                  _loaded           { false };
        bool                                  _play             { false };
        bool                                  _weightsDirty     { true };
        float                                 _timeAccum        { 0.0f };
        std::size_t                           _frameIndex       { 0 };
        std::size_t                           _lastFrameIndex   { static_cast<std::size_t>(-1) };
        float                                 _skeletonScale    { 0.02f };
        int                                   _heatIterations { 20 };
        float                                 _heatLambda { 0.6f };
        float                                 _heatAnchorRadius { 0.05f };
        int                                   _componentMaxJoints { 2 };
        std::array<char, 260>                 _meshPath         {};
        std::array<char, 260>                 _bvhPath          {};

        Engine::SurfaceMesh                   _bindMesh;
        Engine::SurfaceMesh                   _sourceMesh;
        Engine::SurfaceMesh                   _customMesh;
        Engine::SurfaceMesh                   _skinnedMesh;
        std::vector<glm::vec3>                _skeletonSegments;
        std::vector<Skinning::Influence>      _weights;
        std::vector<glm::mat4>                _invBind;

        void                                  ResetModel();
        void                                  UpdateAlignedMesh();

        char const *                GetModelName(std::size_t const i) const { return Content::ModelNames[std::size_t(_models[i])].c_str(); }
        Engine::SurfaceMesh const & GetModelMesh(std::size_t const i) const { return Content::ModelMeshes[std::size_t(_models[i])]; }
    };
} // namespace VCX::Labs::Final
