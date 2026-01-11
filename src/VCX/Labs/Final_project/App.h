#pragma once

#include <vector>

#include "Engine/app.h"
#include "Labs/Final_project/CaseFinal.h"
#include "Labs/Final_project/CaseSmoothing.h"
#include "Labs/Common/UI.h"

namespace VCX::Labs::Final {

    class App : public Engine::IApp {
    private:
        Viewer      _viewer;
        CaseFinal   _caseFinal;
        CaseSmoothing _caseModel;

        std::size_t _caseId { 0 };

        std::vector<std::reference_wrapper<Common::ICase>> _cases;
        Common::UI _ui;

    public:
        App();
        void OnFrame() override;
    };
}
