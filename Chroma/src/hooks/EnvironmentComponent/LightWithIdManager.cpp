#include "ChromaController.hpp"
#include "ChromaLogger.hpp"
#include "colorizer/LightColorizer.hpp"
#include "hooks/LightWithIdManager.hpp"

#include "beatsaber-hook/shared/arrayw.hpp"

#include <algorithm>

#include "GlobalNamespace/LightWithIdManager.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"

std::unordered_map<GlobalNamespace::ILightWithId*, int> Chroma::LightIdRegisterer::RequestedIDs;
std::unordered_set<GlobalNamespace::ILightWithId*> Chroma::LightIdRegisterer::NeedToRegister;
GlobalNamespace::LightWithIdManager* Chroma::LightIdRegisterer::lightWithIdManager;
bool Chroma::LightIdRegisterer::canUnregister = false;

using namespace GlobalNamespace;
using namespace Chroma;

MAKE_HOOK_MATCH(LightWithIdManager_RegisterLight, &LightWithIdManager::RegisterLight, void, LightWithIdManager* self,
                GlobalNamespace::ILightWithId* lightWithId) {
  // Do nothing if Chroma shouldn't run
  if (!ChromaController::GetChromaLegacy() && !ChromaController::DoChromaHooks()) {
    LightWithIdManager_RegisterLight(self, lightWithId);
    return;
  }

  std::string sceneName = self->get_gameObject()->get_scene().get_name();
  if (sceneName.find("Menu") != std::string::npos) {
    LightWithIdManager_RegisterLight(self, lightWithId);
    return;
  }

  LightIdRegisterer::lightWithIdManager = self;

  if (lightWithId->get_isRegistered()) {
    return;
  }

  auto lightId = lightWithId->get_lightId();
  if (lightId < 0 || lightId >= self->_oldMapping.size() ||
      (lightWithId->get_groupId() != -1 && lightWithId->get_elementId() != -1)) {
    LightWithIdManager_RegisterLight(self, lightWithId);
    return;
  }

  auto* lightMap = self->_oldMapping[lightId];
  if (!lightMap) {
    lightMap = LightWithIdManager::LightMapData::New_ctor();
    self->_oldMapping[lightId] = lightMap;
  }
  if (!lightMap->lightInstances) {
    lightMap->lightInstances = ArrayW<ILightWithId*>::New();
  }

  lightWithId->__SetIsRegistered();

  for (auto* light : lightMap->lightInstances) {
    if (light == lightWithId) {
      return;
    }
  }

  int index = lightMap->lightInstances.size();

  auto registerIt = LightIdRegisterer::NeedToRegister.find(lightWithId);
  ChromaLogger::Logger.fmtLog<Paper::LogLevel::INF>("Queueing register type {} index {} {}", lightWithId->get_lightId(), index,
                                                    LightIdRegisterer::NeedToRegister.size());

  if (registerIt != LightIdRegisterer::NeedToRegister.end()) {
    LightIdRegisterer::NeedToRegister.erase(registerIt);
    
    std::optional<int> tableId;
    auto requestedIt = LightIdRegisterer::RequestedIDs.find(lightWithId);
    if (requestedIt != LightIdRegisterer::RequestedIDs.end()) {
      tableId = requestedIt->second;
    }
    
    LightIDTableManager::RegisterIndex(lightId, index, tableId);
  }

  // this also colors the light
  LightColorizer::CreateLightColorizerContractByLightID(lightId, [index, lightWithId](LightColorizer& n) {
    n._lightSwitchEventEffect->RegisterLight(lightWithId, index);
  });

  auto lights = lightMap->lightInstances;
  auto newLights = ArrayW<ILightWithId*>::New(lights.size() + 1);
  std::copy(lights.begin(), lights.end(), newLights.begin());
  newLights[lights.size()] = lightWithId;
  lightMap->lightInstances = newLights;

  // Match PC: apply the manager's cached color after Chroma initializes the tween.
  lightWithId->ColorWasSet(lightMap->hasColor ? lightMap->color : Sombrero::FastColor::clear());
}

// This breaks if it doesn't run after Awake
// PC uses Affinity Patches which run after Awake
MAKE_HOOK_MATCH(LightWithIdManager_UnregisterLight, &LightWithIdManager::UnregisterLight, void, LightWithIdManager* self,
                GlobalNamespace::ILightWithId* lightWithId) {
  // Do nothing if Chroma shouldn't run
  if (!ChromaController::DoChromaHooks() || !Chroma::LightIdRegisterer::canUnregister ||
      (lightWithId->get_groupId() != -1 && lightWithId->get_elementId() != -1)) {
    LightWithIdManager_UnregisterLight(self, lightWithId);
    return;
  }

  lightWithId->__SetIsUnRegistered();
}

MAKE_HOOK_MATCH(LightWithIdManager_LateUpdate, &LightWithIdManager::LateUpdate, void, LightWithIdManager* self) {
  LightWithIdManager_LateUpdate(self);
  // Do nothing if Chroma shouldn't run
  if (!ChromaController::DoChromaHooks()) {
    return;
  }

  LightIdRegisterer::lightWithIdManager = self;
  //    LightWithIdManager_LateUpdate(self);

  //    using LightList = System::Collections::Generic::List_1<GlobalNamespace::ILightWithId *>;
  //
  //    static const MethodInfo *Clear_info = il2cpp_utils::FindMethod(self->lightsToUnregister, "Clear",
  //    std::vector<Il2CppClass*>{}, ::std::vector<const Il2CppType*>{}); static auto Clear = reinterpret_cast<void
  //    (*)(LightList *)>(Clear_info->methodPointer);
  //
  //    // For some reason this doesnt get emptied and is continuously looped over.
  //    // When unregistering a large amount of lights in Chroma, this can add lag, so we dump the list ourselves.
  //    Clear(self->lightsToUnregister);
}

MAKE_HOOK_MATCH(LightWithIdManager_SetColorForId, &LightWithIdManager::SetColorForId, void, LightWithIdManager* self, int lightId,
                ::UnityEngine::Color color) {
  // Do nothing if Chroma shouldn't run
  if (!ChromaController::DoChromaHooks()) {
    LightWithIdManager_SetColorForId(self, lightId, color);
    return;
  }

  std::string sceneName = self->get_gameObject()->get_scene().get_name();
  if (sceneName.find("Menu") != std::string::npos) {
    LightWithIdManager_SetColorForId(self, lightId, color);
    return;
  }

  if (lightId < 0 || lightId >= self->_oldMapping.size()) return;
  auto* lightMap = self->_oldMapping[lightId];
  if (!lightMap) {
    lightMap = LightWithIdManager::LightMapData::New_ctor();
    self->_oldMapping[lightId] = lightMap;
  }
  if (!lightMap->lightInstances) {
    lightMap->lightInstances = ArrayW<ILightWithId*>::New();
  }
  lightMap->color = color;
  lightMap->hasColor = true;
  self->_didChangeSomeColorsThisFrame = true;
  auto list = lightMap->lightInstances;
  for (auto const& lightWithId : list) {
    if ((lightWithId != nullptr) && lightWithId->get_isRegistered()) {
      lightWithId->ColorWasSet(color);
    }
  }
}

void LightWithIdManagerHook() {
  INSTALL_HOOK(ChromaLogger::Logger, LightWithIdManager_LateUpdate);
  INSTALL_HOOK(ChromaLogger::Logger, LightWithIdManager_UnregisterLight);
  INSTALL_HOOK(ChromaLogger::Logger, LightWithIdManager_SetColorForId);
  INSTALL_HOOK(ChromaLogger::Logger, LightWithIdManager_RegisterLight);
}

ChromaInstallHooks(LightWithIdManagerHook)
