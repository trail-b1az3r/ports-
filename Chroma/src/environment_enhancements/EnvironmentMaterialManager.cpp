#include "beatsaber-hook/shared/safeptr.hpp"
#include "environment_enhancements/EnvironmentMaterialManager.hpp"

#include "UnityEngine/AddressableAssets/Addressables.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Object.hpp"
#include "System/String.hpp"
#include "UnityEngine/ResourceManagement/AsyncOperations/AsyncOperationHandle_1.hpp"
#include "UnityEngine/ResourceManagement/ResourceProviders/SceneInstance.hpp"
#include "UnityEngine/ResourceManagement/ResourceProviders/SceneReleaseMode.hpp"
#include "UnityEngine/HideFlags.hpp"
#include "ChromaLogger.hpp"

safe_ptr<UnityEngine::Shader*> Chroma::EnvironmentMaterialManager::waterLit;

custom_types::Helpers::Coroutine Chroma::EnvironmentMaterialManager::Activate() {
  using namespace Sombrero::Linq::Functional;

  CJDLogger::Logger.fmtLog<Paper::LogLevel::INF>("Waiting to scene load");

  for (int i = 0; i < 90 * 2; i++) {
    co_yield nullptr;
  }

  CJDLogger::Logger.fmtLog<Paper::LogLevel::INF>("Beginning scene load");

  auto Load = [](std::string_view environmentName) {
    CJDLogger::Logger.fmtLog<Paper::LogLevel::INF>("Loading environment [{}].", environmentName);
    return UnityEngine::AddressableAssets::Addressables::LoadSceneAsync(
        static_cast<System::String*>(StringW(environmentName)), UnityEngine::SceneManagement::LoadSceneMode::Additive,
        true, 100, UnityEngine::ResourceManagement::ResourceProviders::SceneReleaseMode::ReleaseSceneWhenSceneUnloaded);
  };

  auto environments =
      std::array<std::string_view, 3>({ "BTSEnvironment", "BillieEnvironment", "InterscopeEnvironment" });
  auto loads = environments | Select([&](std::string_view s) { return Load(s); });
  std::vector<UnityEngine::SceneManagement::Scene> scenes;
  for (auto n : loads) {
    if (n.m_InternalOp == nullptr) {
      continue;
    }

    while (!n.IsDone) {
      co_yield nullptr;
    }
    scenes.push_back(n.Result.Scene);
  }

  auto environmentMaterials = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Material*>();

  auto Save = [&](ShaderType key, std::string_view matName) {
    auto* material = environmentMaterials.front_or_default([&](auto const& e) { return e->get_name() == matName; });
    if (material != nullptr) {
      EnvironmentMaterials[key] = UnityEngine::Material::New_ctor(material);
      EnvironmentMaterials[key]->hideFlags = UnityEngine::HideFlags::DontUnloadUnusedAsset;
      // TODO: Check if this is necessary
      UnityEngine::Object::DontDestroyOnLoad(EnvironmentMaterials[key].ptr());
      CJDLogger::Logger.fmtLog<Paper::LogLevel::INF>("Saving [{}] to [{}].", matName, static_cast<int>(key));
    } else {
      CJDLogger::Logger.fmtLog<Paper::LogLevel::INF>("Could not find [{}].", matName);
    }
  };

  Save(ShaderType::BTSPillar, "BTSDarkEnvironmentWithHeightFog");
  Save(ShaderType::BillieWater, "WaterfallFalling");
  Save(ShaderType::WaterfallMirror, "WaterfallMirror");
  Save(ShaderType::InterscopeConcrete, "Concrete2");
  Save(ShaderType::InterscopeCar, "Car");

  auto* waterShader = UnityEngine::Resources::FindObjectsOfTypeAll<UnityEngine::Shader*>().front_or_default(
      [](auto* shader) { return shader->get_name() == "Custom/WaterLit"; });
  if (waterShader) EnvironmentMaterialManager::waterLit = UnityEngine::Object::Instantiate(waterShader);

  for (auto const& scene : scenes) {
    UnityEngine::SceneManagement::SceneManager::UnloadSceneAsync(scene);
  }

  co_return;
}

std::optional<UnityEngine::Material*> Chroma::EnvironmentMaterialManager::getMaterial(Chroma::ShaderType shaderType) {
  auto it = EnvironmentMaterials.find(shaderType);

  if (it == EnvironmentMaterials.end()) {
    return std::nullopt;
  }

  auto material = it->second;

  if (!static_cast<bool>(material)) {
    ChromaLogger::Logger.error("Material for shader type {} is no longer alive", static_cast<int>(shaderType));
    return std::nullopt;
  }

  return it->second.ptr();
}
