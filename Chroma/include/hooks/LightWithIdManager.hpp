#pragma once

#include <unordered_set>
#include <unordered_map>

#include "ChromaLogger.hpp"
#include "colorizer/LightColorizer.hpp"

#include "GlobalNamespace/ILightWithId.hpp"
#include "GlobalNamespace/LightWithIdManager.hpp"
#include "custom-json-data/shared/VList.h"
#include "lighting/LightIDTableManager.hpp"

namespace Chroma {
struct LightIdRegisterer {
  static std::unordered_map<GlobalNamespace::ILightWithId*, int> RequestedIDs;
  static std::unordered_set<GlobalNamespace::ILightWithId*> NeedToRegister;
  static GlobalNamespace::LightWithIdManager* lightWithIdManager;
  static bool canUnregister;

  static void Reset() {
    RequestedIDs.clear();
    NeedToRegister.clear();
    lightWithIdManager = nullptr;
    canUnregister = false;
  }

  static void SetRequestedId(GlobalNamespace::ILightWithId* lightWithId, int id) {
    RequestedIDs[lightWithId] = id;
  }

  static void MarkForTableRegister(GlobalNamespace::ILightWithId* lightWithId) {
    NeedToRegister.emplace(lightWithId);
  }

  static void ForceUnregister(GlobalNamespace::ILightWithId* lightWithId) {
    int lightId = lightWithId->get_lightId();
    if (!lightWithIdManager || lightId < 0 || lightId >= lightWithIdManager->_oldMapping.size()) return;
    auto* lightMap = lightWithIdManager->_oldMapping[lightId];
    if (!lightMap) return;
    auto lights = lightMap->lightInstances;
    if (!lights) return;
    int index = -1;

    for (auto i = 0; i < lights.size(); i++) {
      auto const& n = lights[i];
      if (n == lightWithId) {
        index = i;
        break;
      }
    }

    if (index < 0) return;
    lights[index] = nullptr;
    LightIDTableManager::UnregisterIndex(lightId, index);
    LightColorizer::CreateLightColorizerContractByLightID(
        lightId, [lightWithId](LightColorizer& n) { n._lightSwitchEventEffect->UnregisterLight(lightWithId); });
    lightWithId->__SetIsUnRegistered();
  }
};
} // namespace Chroma
