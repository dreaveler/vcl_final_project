#pragma once

#include <array>
#include <string>

#include "Assets/bundled.h"
#include "Engine/SurfaceMesh.h"

namespace VCX::Labs::Final {
    class Content {
    public:
        static std::array<Engine::SurfaceMesh, Assets::ExampleModels.size()> const ModelMeshes;
        static std::array<std::string,         Assets::ExampleModels.size()> const ModelNames;
    };
}
