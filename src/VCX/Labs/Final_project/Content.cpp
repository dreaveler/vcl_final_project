#include <filesystem>

#include <spdlog/spdlog.h>

#include "Engine/loader.h"
#include "Labs/Final_project/Content.h"

namespace VCX::Labs::Final {
    static std::array<Engine::SurfaceMesh, Assets::ExampleModels.size()> LoadExampleModels() {
        std::array<Engine::SurfaceMesh, Assets::ExampleModels.size()> models;
        for (std::size_t i = 0; i < models.size(); i++) {
            models[i] = Engine::LoadSurfaceMesh(Assets::ExampleModels[i], true);
            models[i].NormalizePositions();
        }
        return models;
    }

    static std::array<std::string, Assets::ExampleModels.size()> LoadExampleNames() {
        std::array<std::string, Assets::ExampleModels.size()> names;
        for (std::size_t i = 0; i < names.size(); i++) {
            names[i] = std::filesystem::path(Assets::ExampleModels[i]).filename().string();
        }
        return names;
    }

    std::array<Engine::SurfaceMesh, Assets::ExampleModels.size()> const Content::ModelMeshes = LoadExampleModels();
    std::array<std::string,         Assets::ExampleModels.size()> const Content::ModelNames  = LoadExampleNames();
} // namespace VCX::Labs::Final
