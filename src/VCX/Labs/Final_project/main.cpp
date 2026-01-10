#include "Assets/bundled.h"
#include "Labs/Final_project/App.h"

int main() {
    using namespace VCX;
    return Engine::RunApp<Labs::Animation::App>(Engine::AppContextOptions {
        .Title      = "VCX Labs Final Project",
        .WindowSize = { 1024, 768 },
        .FontSize   = 16,
        .IconFileNames = Assets::DefaultIcons,
        .FontFileNames = Assets::DefaultFonts,
    });
}
