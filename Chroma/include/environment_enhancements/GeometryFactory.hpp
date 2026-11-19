#include "beatsaber-hook/shared/safeptr.hpp"
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>

#include "beatsaber-hook/shared/rapidjson.hpp"
#include "GameObjectInfo.hpp"
#include "sombrero/shared/FastColor.hpp"

#include "UnityEngine/Material.hpp"
#include "GlobalNamespace/TubeBloomPrePassLight.hpp"
#include "GlobalNamespace/LightWithIdManager.hpp"
#include "MaterialsManager.hpp"

namespace Zenject {
class DiContainer;
}

namespace Chroma {

enum struct GeometryType { Sphere, Capsule, Cylinder, Cube, Plane, Quad, Triangle };

class GeometryFactory {
public:
  GeometryFactory(MaterialsManager& materialsManager, bool v2, Zenject::DiContainer* container);

  MaterialsManager& materialsManager;
  bool v2;

  UnityEngine::GameObject* Create(rapidjson::Value const& data);

private:
  std::optional<GlobalNamespace::TubeBloomPrePassLight*> _originalTubeBloomPrePassLight = std::nullopt;
  safe_ptr<GlobalNamespace::LightWithIdManager*> lightWithIDManager;
};
} // namespace Chroma
