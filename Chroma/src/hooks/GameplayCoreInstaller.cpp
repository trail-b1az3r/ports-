#include "ChromaHooks.hpp"
#include "ChromaLogger.hpp"

#include "ChromaController.hpp"

#include "GlobalNamespace/GameplayCoreInstaller.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "System/Threading/Tasks/Task.hpp"

MAKE_HOOK_MATCH(GameplayCoreSceneSetupData_LoadTransformedBeatmapDataAsync,
                &GlobalNamespace::GameplayCoreSceneSetupData::LoadTransformedBeatmapDataAsync,
                System::Threading::Tasks::Task*, GlobalNamespace::GameplayCoreSceneSetupData* self) {
  if (Chroma::ChromaController::DoChromaHooks()) {
    // Rebuild the mutable track state on restart/replay, as the old game did.
    self->_enableBeatmapDataCaching = false;
  }
  return GameplayCoreSceneSetupData_LoadTransformedBeatmapDataAsync(self);
}

MAKE_HOOK_MATCH(InstallBindings, &GlobalNamespace::GameplayCoreInstaller::InstallBindings, void,
                GlobalNamespace::GameplayCoreInstaller* self) {
  // Do nothing if Chroma shouldn't run
  Chroma::ChromaController::GameplayCoreInstaller = std::nullopt;
  if (!Chroma::ChromaController::DoChromaHooks()) {
    InstallBindings(self);
    return;
  }


  InstallBindings(self);

  Chroma::ChromaController::GameplayCoreInstaller = self;
}

void InstallGameplayCoreInstallerHooks() {
  INSTALL_HOOK(ChromaLogger::Logger, GameplayCoreSceneSetupData_LoadTransformedBeatmapDataAsync);
  INSTALL_HOOK(ChromaLogger::Logger, InstallBindings);
}

ChromaInstallHooks(InstallGameplayCoreInstallerHooks);
