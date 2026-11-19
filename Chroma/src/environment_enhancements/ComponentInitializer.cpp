#include "beatsaber-hook/shared/safeptr.hpp"
#include "ChromaLogger.hpp"
#include "main.hpp"
#include "environment_enhancements/ComponentInitializer.hpp"
#include "environment_enhancements/EnvironmentEnhancementManager.hpp"
#include "colorizer/LightColorizer.hpp"
#include "hooks/TrackLaneRingsManager.hpp"
#include "lighting/ChromaRingsRotationEffect.hpp"
#include "lighting/ChromaFogController.hpp"

#include "GlobalNamespace/LightWithIdMonoBehaviour.hpp"
#include "GlobalNamespace/LightWithIds.hpp"
#include "GlobalNamespace/ILightWithId.hpp"
#include "GlobalNamespace/TrackLaneRing.hpp"
#include "GlobalNamespace/TrackLaneRingsManager.hpp"
#include "GlobalNamespace/TrackLaneRingsPositionStepEffectSpawner.hpp"
#include "GlobalNamespace/TrackLaneRingsRotationEffectSpawner.hpp"
#include "GlobalNamespace/TrackLaneRingsRotationEffect.hpp"
#include "GlobalNamespace/Spectrogram.hpp"
#include "GlobalNamespace/LightRotationEventEffect.hpp"
#include "GlobalNamespace/LightPairRotationEventEffect.hpp"
#include "GlobalNamespace/ParticleSystemEventEffect.hpp"
#include "GlobalNamespace/Mirror.hpp"
#include "GlobalNamespace/MirrorRendererSO.hpp"
#include "GlobalNamespace/BloomFogEnvironment.hpp"
#include "GlobalNamespace/BloomFogEnvironmentParams.hpp"
#include "GlobalNamespace/TubeBloomPrePassLight.hpp"

#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/ParticleSystem.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Material.hpp"

#include "System/Collections/Generic/List_1.hpp"
#include "System/Array.hpp"

#include "Zenject/DiContainer.hpp"

#include "beatsaber-hook/shared/api.hpp"
#include "beatsaber-hook/shared/members.hpp"
#include "beatsaber-hook/shared/types.hpp"
#include "beatsaber-hook/shared/utils.hpp"
#include "hooks/LightWithIdManager.hpp"
#include "utils/ChromaUtils.hpp"

#include "tracks/shared/Animation/GameObjectTrackController.hpp"

#include <functional>

#include "beatsaber-hook/shared/types.hpp"

#include "sombrero/shared/linq_functional.hpp"


using namespace GlobalNamespace;
using namespace Chroma;
using namespace Sombrero::Linq::Functional;

GameObjectInfo const& Chroma::ComponentInitializer::InitializeComponents(UnityEngine::Transform* root, UnityEngine::Transform* original,
                                                                         std::vector<GameObjectInfo>& gameObjectInfos,
                                                                         std::vector<std::shared_ptr<IComponentData>>& componentDatas,
                                                                         Zenject::DiContainer* _container) {

  ArrayW<UnityEngine::MonoBehaviour*> rootComponents = root->GetComponents<UnityEngine::MonoBehaviour*>();
  ArrayW<UnityEngine::MonoBehaviour*> otherComponents = original->GetComponents<UnityEngine::MonoBehaviour*>();

  for (int i = 0; i < rootComponents.size(); i++) {
    auto* rootComp = rootComponents.at(i);
    auto* otherComp = otherComponents.at(i);

    _container->Inject(rootComp);

    if (auto transformController = i2c::try_cast<Tracks::GameObjectTrackController*>(rootComp)) {
      UnityEngine::Object::DestroyImmediate(rootComp);
    }

    if (auto lightWithIdMonoBehaviour = i2c::try_cast<LightWithIdMonoBehaviour*>(rootComp)) {
      LightIdRegisterer::MarkForTableRegister(lightWithIdMonoBehaviour->i___GlobalNamespace__ILightWithId());
    }

    if (auto lightsWithIds = i2c::try_cast<LightWithIds*>(rootComp)) {
      if (lightsWithIds->lightWithIds) {
        // The game's LightWithIds implementations return reference arrays.
        for (auto* light : ArrayW<LightWithIds::LightWithId*>(i2c::cast<System::Array*>(lightsWithIds->lightWithIds))) {
          LightIdRegisterer::MarkForTableRegister(light->i___GlobalNamespace__ILightWithId());
        }
      }
    }

    if (auto trackLaneRing = i2c::try_cast<TrackLaneRing*>(rootComp)) {
      auto* originalTrackLaneRing = i2c::cast<TrackLaneRing*>(otherComp);

      auto ringIt = EnvironmentEnhancementManager::RingRotationOffsets.find(originalTrackLaneRing);
      if (ringIt != EnvironmentEnhancementManager::RingRotationOffsets.end()) {
        EnvironmentEnhancementManager::RingRotationOffsets[trackLaneRing] = ringIt->second;
      }

      trackLaneRing->_transform = root;
      trackLaneRing->_positionOffset = originalTrackLaneRing->_positionOffset;
      trackLaneRing->_posZ = originalTrackLaneRing->_posZ;

      TrackLaneRingsManager* managerToAdd = nullptr;

      for (auto const& manager : Chroma::TrackLaneRingsManagerHolder::RingManagers) {
        // TrackLaneRingsManagerComponentData? componentData = componentDatas
        //   .OfType<TrackLaneRingsManagerComponentData>()
        //   .FirstOrDefault(n => n.OldTrackLaneRingsManager == manager);
        std::optional<TrackLaneRingsManagerComponentData*> componentData;
        for (auto const& componentDataC : componentDatas) {
          if (componentDataC->getComponentType() == ComponentType::TrackLaneRingsManager) {
            auto trackLaneData = std::static_pointer_cast<TrackLaneRingsManagerComponentData>(componentDataC);

            if ((GlobalNamespace::TrackLaneRingsManager*)trackLaneData->OldTrackLaneRingsManager == manager) {
              componentData = trackLaneData.get();
              break;
            }
          }
        }

        if (componentData) {
          managerToAdd = (GlobalNamespace::TrackLaneRingsManager*)componentData.value()->NewTrackLaneRingsManager;
        } else {
          auto rings = manager->_rings;
          if (rings.contains(originalTrackLaneRing)) {
            managerToAdd = manager;
          }
        }

        if (managerToAdd != nullptr) {
          auto rings = managerToAdd->_rings;

          if (rings) {
            std::vector<UnityW<GlobalNamespace::TrackLaneRing>> newRingList(rings.begin(), rings.end());
            newRingList.emplace_back(trackLaneRing);

            managerToAdd->_rings = ArrayW<UnityW<GlobalNamespace::TrackLaneRing>>(newRingList);
          } else {
            managerToAdd->_rings = ArrayW<UnityW<GlobalNamespace::TrackLaneRing>>({trackLaneRing});
          }

          break;
        }
      }
    }

    if (auto trackLaneRingsPositionStepEffectSpawner = i2c::try_cast<TrackLaneRingsPositionStepEffectSpawner*>(rootComp)) {
      for (auto const& manager : TrackLaneRingsManagerHolder::RingManagers) {
        std::optional<TrackLaneRingsManagerComponentData*> componentData;

        // TrackLaneRingsManagerComponentData? componentData = componentDatas
        //   .OfType<TrackLaneRingsManagerComponentData>()
        //   .FirstOrDefault(n => n.OldTrackLaneRingsManager == manager);
        for (auto const& componentDataC : componentDatas) {
          if (componentDataC->getComponentType() == ComponentType::TrackLaneRingsManager) {
            auto trackLaneData = std::static_pointer_cast<TrackLaneRingsManagerComponentData>(componentDataC);

            if ((GlobalNamespace::TrackLaneRingsManager*)trackLaneData->OldTrackLaneRingsManager == manager) {
              componentData = trackLaneData.get();
              break;
            }
          }
        }

        if (componentData) {
          trackLaneRingsPositionStepEffectSpawner->_trackLaneRingsManager =
              (GlobalNamespace::TrackLaneRingsManager*)componentData.value()->NewTrackLaneRingsManager;
          break;
        }
      }
    }
    if (auto trackLaneRingsRotationEffectSpawner = i2c::try_cast<TrackLaneRingsRotationEffectSpawner*>(rootComp)) {
      trackLaneRingsRotationEffectSpawner->_trackLaneRingsRotationEffect = root->GetComponent<TrackLaneRingsRotationEffect*>();
    }
    if (auto spectrogram = i2c::try_cast<Spectrogram*>(rootComp)) {
      spectrogram->_meshRenderers = root->GetComponentsInChildren<UnityW<UnityEngine::MeshRenderer>>(true);
    }
    if (auto lightPairRotationEventEffect = i2c::try_cast<LightPairRotationEventEffect*>(rootComp)) {
      auto originalLightPairRotationEventEffect = i2c::cast<LightPairRotationEventEffect*>(otherComp);

      auto transformL = originalLightPairRotationEventEffect->_transformL;
      auto transformR = originalLightPairRotationEventEffect->_transformR;

      lightPairRotationEventEffect->_transformL = root->GetChild(transformL->GetSiblingIndex());
      lightPairRotationEventEffect->_transformR = root->GetChild(transformR->GetSiblingIndex());

      // We have to enable the object to tell unity to run Start
      lightPairRotationEventEffect->set_enabled(true);
    } else if (auto particleSystemEventEffect = i2c::try_cast<ParticleSystemEventEffect*>(rootComp)) {
      particleSystemEventEffect->_particleSystem = root->GetComponent<UnityEngine::ParticleSystem*>();
      particleSystemEventEffect->set_enabled(true);
    } else if (auto mirror = i2c::try_cast<Mirror*>(rootComp)) {
      mirror->_renderer = root->GetComponent<UnityEngine::MeshRenderer*>();
    }
  }

  auto const& newGameObject = gameObjectInfos.emplace_back(root->get_gameObject());

  auto rootChildCount = root->get_childCount();

  for (int i = 0; i < rootChildCount; i++) {
    auto transform = root->GetChild(i);

    int index = transform->GetSiblingIndex();
    InitializeComponents(transform, original->GetChild(index), gameObjectInfos, componentDatas, _container);
  }

  return newGameObject;
}

void ComponentInitializer::PrefillComponentsData(UnityEngine::Transform* root,
                                                 std::vector<std::shared_ptr<IComponentData>>& componentDatas) {
  auto* trackLaneRingsManager = root->GetComponent<GlobalNamespace::TrackLaneRingsManager*>();
  if (trackLaneRingsManager != nullptr) {
    std::shared_ptr<TrackLaneRingsManagerComponentData> manager = std::make_shared<TrackLaneRingsManagerComponentData>();
    manager->OldTrackLaneRingsManager.emplace(trackLaneRingsManager);
    componentDatas.emplace_back(manager);
  }

  componentDatas.reserve(componentDatas.size() + root->get_childCount());
  auto rootChildCount = root->get_childCount();
  for (int i = 0; i < rootChildCount; i++) {
    auto transform = root->GetChild(i);
    PrefillComponentsData(transform, componentDatas);
  }
}

void ComponentInitializer::PostfillComponentsData(UnityEngine::Transform* root, UnityEngine::Transform* original,
                                                  std::vector<std::shared_ptr<IComponentData>> const& componentDatas) {
  auto* trackLaneRingsManager = root->GetComponent<GlobalNamespace::TrackLaneRingsManager*>();
  if (trackLaneRingsManager != nullptr) {
    Chroma::TrackLaneRingsManagerHolder::RingManagers.emplace_back(trackLaneRingsManager);
    auto* originalManager = original->GetComponent<TrackLaneRingsManager*>();

    for (auto const& componentData : componentDatas) {
      if (componentData->getComponentType() == ComponentType::TrackLaneRingsManager) {
        auto trackLaneData = std::static_pointer_cast<TrackLaneRingsManagerComponentData>(componentData);

        if ((GlobalNamespace::TrackLaneRingsManager*)trackLaneData->OldTrackLaneRingsManager == originalManager) {
          trackLaneData->NewTrackLaneRingsManager.emplace(trackLaneRingsManager);
        }
      }
    }
  }

  auto* rotationEffect = root->GetComponent<GlobalNamespace::TrackLaneRingsRotationEffect*>();
  if (rotationEffect != nullptr) {
    UnityEngine::Object::Destroy(rotationEffect);
  }

  auto rootChildCount = root->get_childCount();
  for (int i = 0; i < rootChildCount; i++) {
    auto transform = root->GetChild(i);
    auto index = transform->GetSiblingIndex();
    PostfillComponentsData(transform, original->GetChild(index), componentDatas);
  }
}

static void InitializeTubeBloomPrePassLights(rapidjson::Value const& data, std::span<UnityEngine::Component* const> comps) {
  auto tubeBloomPrePassLightJSON =
      ChromaUtils::getIfExists<rapidjson::Value::ConstObject>(data, Chroma::NewConstants::TUBE_BLOOM_PRE_PASS_LIGHT);
  if (!tubeBloomPrePassLightJSON) {
    return;
  }

  auto colorAlphaMultiplier = ChromaUtils::getIfExists<float>(*tubeBloomPrePassLightJSON, Chroma::NewConstants::COLOR_ALPHA_MULTIPLIER);
  auto bloomFogIntensityMultiplier =
      ChromaUtils::getIfExists<float>(*tubeBloomPrePassLightJSON, Chroma::NewConstants::BLOOM_FOG_INTENSITY_MULTIPLIER);

  auto SetColorAlphaMultiplier = [](TubeBloomPrePassLight* tubeBloomPrePassLight, float value) constexpr {
    tubeBloomPrePassLight->_parametricBoxControllerOnceParInitialized = false;
    tubeBloomPrePassLight->_bakedGlowOnceParInitialized = false;

    tubeBloomPrePassLight->_colorAlphaMultiplier = value;
    tubeBloomPrePassLight->MarkDirty();
  };

  for (auto const& comp : comps) {
    auto tubeCast = i2c::try_cast<TubeBloomPrePassLight*>(comp);
    if (!tubeCast) {
      continue;
    }
    auto* tubeBloomPrePassLight = tubeCast;

    if (colorAlphaMultiplier) {
      SetColorAlphaMultiplier(tubeBloomPrePassLight, *colorAlphaMultiplier);
    }
    if (bloomFogIntensityMultiplier) {
      tubeBloomPrePassLight->bloomFogIntensityMultiplier = *bloomFogIntensityMultiplier;
    }
  }
}

static void InitializeFog(rapidjson::Value const& data, std::span<UnityEngine::Component* const> comps) {
  auto bloomJSON = ChromaUtils::getIfExists<rapidjson::Value::ConstObject>(data, Chroma::NewConstants::BLOOM_FOG_ENVIRONMENT);

  if (!bloomJSON) {
    return;
  }

  auto attenuation = ChromaUtils::getIfExists<float>(*bloomJSON, Chroma::NewConstants::ATTENUATION);
  auto offset = ChromaUtils::getIfExists<float>(*bloomJSON, Chroma::NewConstants::OFFSET);
  auto startY = ChromaUtils::getIfExists<float>(*bloomJSON, Chroma::NewConstants::HEIGHT_FOG_STARTY);
  auto height = ChromaUtils::getIfExists<float>(*bloomJSON, Chroma::NewConstants::HEIGHT_FOG_HEIGHT);

  for (auto* comp : comps) {
    auto fogCast = i2c::try_cast<BloomFogEnvironment*>(comp);
    if (!fogCast) {
      continue;
    }

    auto* fog = fogCast;

    auto params = fog->_fogParams;
    if (attenuation) {
      params->attenuation = Chroma::fogAttenuationFix(*attenuation);
    }
    if (offset) {
      params->offset = *offset;
    }
    if (startY) {
      params->heightFogStartY = *startY;
    }
    if (height) {
      params->heightFogHeight = *height;
    }
  }
}

static void InitializeLights(rapidjson::Value const& data, std::span<UnityEngine::Component* const> comps, bool v2) {
  auto lightDataJSON = ChromaUtils::getIfExists<rapidjson::Value::ConstObject>(data, Chroma::NewConstants::LIGHT_WITH_ID);
  if (!v2 && !lightDataJSON) {
    return;
  }

  /// JSON
  std::optional<int> lightID;
  std::optional<int> type;
  if (v2) {
    lightID = ChromaUtils::getIfExists<int>(data, NewConstants::V2_LIGHT_ID);
  } else {
    lightID = ChromaUtils::getIfExists<int>(*lightDataJSON, NewConstants::LIGHT_ID);
    type = ChromaUtils::getIfExists<int>(*lightDataJSON, NewConstants::LIGHT_TYPE);
  }

  if (!type && !lightID) {
    return;
  }

  /// GRAB ALL LIGHT IDS
  std::vector<ILightWithId*> lightWithIds;

  for (auto const& comp : comps) {
    if (comp == nullptr) {
      continue;
    }

    if (auto castedLights = i2c::try_cast<LightWithIds*>(comp)) {
      if (castedLights->lightWithIds == nullptr) {
        continue;
      }

      for (auto* light : ArrayW<LightWithIds::LightWithId*>(i2c::cast<System::Array*>(castedLights->lightWithIds))) {
        lightWithIds.emplace_back(light->i___GlobalNamespace__ILightWithId());
      }
    } else if (auto castedMonoLight = i2c::try_cast<LightWithIdMonoBehaviour*>(comp)) {
      lightWithIds.emplace_back(castedMonoLight->i___GlobalNamespace__ILightWithId());
    }
  }

  if (lightWithIds.empty()) {
    return;
  }

  /// DO THE STUFF
  auto SetType = [&](ILightWithId* const& lightWithId) constexpr {
    if (!type) {
      return;
    }

    auto colorizer = LightColorizer::GetLightColorizer(*type);
    if (!colorizer) {
      ChromaLogger::Logger.fmtLog<Paper::LogLevel::WRN>("No colorizer found for type {}", *type);
      ChromaLogger::Logger.fmtLog<Paper::LogLevel::INF>("Skipping setting type for light {}", fmt::ptr(lightWithId));
      return;
    }

    int psuedoLightId = colorizer->_lightSwitchEventEffect->_lightsID;

    auto monoBehaviourCast = i2c::try_cast<LightWithIdMonoBehaviour*>(lightWithId);

    if (monoBehaviourCast) {
      monoBehaviourCast->_ID = psuedoLightId;
    } 
    auto lightWithIdsCast = i2c::try_cast<LightWithIds::LightWithId*>(lightWithId);

    if (lightWithIdsCast) {
      lightWithIdsCast->_lightId = psuedoLightId;
    }
  };

  auto SetLightID = [&](ILightWithId* const& lightWithId) constexpr {
    if (lightID) {
      ChromaLogger::Logger.fmtLog<Paper::LogLevel::INF>("Setting light ID {} for light {}", *lightID, fmt::ptr(lightWithId));
      LightIdRegisterer::SetRequestedId(lightWithId, *lightID);
    }
  };

  for (auto const& lightWithId : lightWithIds) {
    ChromaLogger::Logger.fmtLog<Paper::LogLevel::INF>("Initializing light with ID {} and type {} {}", lightID.value_or(-1),
                                                      type.value_or(-1), lightWithId->get_isRegistered());

    if (lightWithId->get_isRegistered()) {
      LightIdRegisterer::ForceUnregister(lightWithId);
      LightIdRegisterer::MarkForTableRegister(lightWithId);
      SetType(lightWithId);
      SetLightID(lightWithId);
      LightIdRegisterer::lightWithIdManager->RegisterLight(lightWithId);
    } else {
      SetType(lightWithId);
      SetLightID(lightWithId);
    }
  }
}

void ComponentInitializer::InitializeCustomComponents(UnityEngine::GameObject* go, rapidjson::Value const& data, bool v2) {
  auto componentData = ChromaUtils::getIfExists<rapidjson::Value::ConstObject>(data, Chroma::NewConstants::COMPONENTS);
  if (!v2 && !componentData) {
    return;
  }

  auto comps = go->GetComponentsInChildren<UnityEngine::Component*>(true);
  safe_ptr<Array<UnityEngine::Component*>*> arr((Array<UnityEngine::Component*>*)comps);

  auto compsFiltered = comps | Where([](auto c) constexpr { return c && c->klass; }) | ToVector();

  //    auto compsProtect = compsFiltered | Select([](auto &&c) constexpr {
  //        return SafePtrUnity(c);
  //    }) | ToVector();

  if (v2) {
    InitializeLights(data, compsFiltered, v2);
    return;
  }
  if (!componentData) {
    return;
  }

  InitializeLights(*componentData, compsFiltered, v2);
  InitializeFog(*componentData, compsFiltered);
  InitializeTubeBloomPrePassLights(*componentData, compsFiltered);
}
