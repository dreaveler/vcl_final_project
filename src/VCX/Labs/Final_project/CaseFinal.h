#pragma once
#include "Engine/GL/Frame.hpp"
#include "Engine/GL/Program.h"
#include "Engine/GL/RenderItem.h"
#include "Engine/loader.h"
#include "Labs/Common/ICase.h"
#include "Labs/Common/ImageRGB.h"
#include "Labs/Common/OrbitCameraManager.h"
#include "ReadBVH.h"
#include "HumanDS.h"
#include <array>
#include <string>

namespace VCX::Labs::Animation {

    class BoxRenderer {
    public:
        BoxRenderer();

        void render(Engine::GL::UniqueProgram & program);
        void calc_vert_position();

        std::vector<glm::vec3>              VertsPosition;
        glm::vec3                           CenterPosition;
        glm::vec3                           MainAxis;
        float                               width = 0.05f;
        float                               length = 0.2f;

        Engine::GL::UniqueIndexedRenderItem BoxItem;  // render the box
        Engine::GL::UniqueIndexedRenderItem LineItem; // render line on box
    };

    // render x, y, z axis
    class BackGroundRender {
    public:
        BackGroundRender();
        void render(Engine::GL::UniqueProgram & program, bool draw_axis);
        void UpdatePoints(const std::vector<glm::vec3> & points);

    public:
        Engine::GL::UniqueIndexedRenderItem LineItem;
        Engine::GL::UniqueRenderItem        PointItem;
    };

    class CaseFinal : public Common::ICase {
    public:
        CaseFinal();

        virtual std::string_view const GetName() override { return "Final Project:Motion Matching"; }

        virtual void                        OnSetupPropsUI() override;
        virtual Common::CaseRenderResult    OnRender(std::pair<std::uint32_t, std::uint32_t> const desiredSize) override;
        virtual void                        OnProcessInput(ImVec2 const & pos) override;

    private:
        void                                BuildBySegments(const std::vector<glm::vec3> & segment_points);
        void                                ResetSystem();

    private:
        Engine::GL::UniqueProgram           _program;
        Engine::GL::UniqueRenderFrame       _frame;
        Engine::Camera                      _camera { .Eye = glm::vec3(-3, 3, 3) };
        Common::OrbitCameraManager          _cameraManager;
        bool                                _loaded { false };
        bool                                _play { false };
        bool                                _showAxis { true };
        bool                                _browseFailed { false };
        unsigned long                       _browseError { 0 };
        float                               _timeAccum { 0.0f };
        std::size_t                         _frameIndex { 0 };
        float                               _scale { 0.025f };

        BackGroundRender                    BackGround;
        std::vector<BoxRenderer>            arms; // for render the arm
        Motion                              _motion;
        std::array<char, 260>               _pathBuffer {};
    };
} // namespace VCX::Labs::Animation
