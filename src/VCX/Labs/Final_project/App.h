#pragma once

#include <vector>

#include "Engine/app.h"
#include "Labs/Final_project/CaseFinal.h"
#include "Labs/Common/UI.h"

namespace VCX::Labs::Animation {

    class App : public Engine::IApp {
    private:
        CaseFinal  _caseFinal;

        std::size_t        _caseId = 0;

        std::vector<std::reference_wrapper<Common::ICase>> _cases = {
            _caseFinal
        };
        Common::UI _ui;

    public:
        App();
        void OnFrame() override;
    };
}
