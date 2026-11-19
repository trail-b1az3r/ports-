#include "Chroma.hpp"

#include "ChromaController.hpp"

#include "GlobalNamespace/TutorialScenesTransitionSetupData.hpp"

using namespace CustomJSONData;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace Chroma;

MAKE_HOOK_MATCH(TutorialScenesTransitionSetupData_Init, &TutorialScenesTransitionSetupData::Init, void,
                TutorialScenesTransitionSetupData* self,
                GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings,
                GlobalNamespace::GameplayAdditionalInformation* gameplayAdditionalInformation) {
  ChromaController::TutorialMode = true;
  TutorialScenesTransitionSetupData_Init(self, playerSpecificSettings, gameplayAdditionalInformation);
}

void TutorialScenesTransitionSetupDataHook() {
  INSTALL_HOOK(ChromaLogger::Logger, TutorialScenesTransitionSetupData_Init);
}

ChromaInstallHooks(TutorialScenesTransitionSetupDataHook)