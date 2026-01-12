#include "Labs/Final_project/App.h"

namespace VCX::Labs::Final {
    App::App() :
        _viewer(),
        _caseFinal(),
        _caseModel(_viewer, { Assets::ExampleModel::Human, Assets::ExampleModel::Dancing }),
        _cases { _caseFinal, _caseModel },
        _ui(Labs::Common::UIOptions { }) {
    }

    void App::OnFrame() {
        _ui.Setup(_cases, _caseId);
    }
}
