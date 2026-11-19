#include "Chroma.hpp"

#include "custom-json-data/shared/CustomBeatmapData.h"
#include "GlobalNamespace/MultiplayerLevelScenesTransitionSetupData.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"

#include "hooks/SceneTransition/SceneTransitionHelper.hpp"

using namespace CustomJSONData;
using namespace GlobalNamespace;
using namespace UnityEngine;
using namespace Chroma;

MAKE_HOOK_MATCH(MultiplayerLevelScenesTransitionSetupData_Init, &MultiplayerLevelScenesTransitionSetupData::InitAndSetupScenes,
                void, MultiplayerLevelScenesTransitionSetupData* self) {

  
  auto customBeatmapLevel = i2c::try_cast<SongCore::SongLoader::CustomBeatmapLevel*>(self->get_beatmapLevel());
  if (!customBeatmapLevel) {
    MultiplayerLevelScenesTransitionSetupData_Init(self);
    return;
  }

  // TODO: Fix environment override
  SceneTransitionHelper::Patch(customBeatmapLevel, self->beatmapKey, self->_environmentInfo.ptr());

  MultiplayerLevelScenesTransitionSetupData_Init(self);
}

void MultiplayerLevelScenesTransitionSetupDataHook() {
  INSTALL_HOOK(ChromaLogger::Logger, MultiplayerLevelScenesTransitionSetupData_Init);
}

ChromaInstallHooks(MultiplayerLevelScenesTransitionSetupDataHook)