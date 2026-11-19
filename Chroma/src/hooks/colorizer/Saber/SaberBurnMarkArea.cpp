#include "Chroma.hpp"
#include "ChromaController.hpp"
#include "GlobalNamespace/SaberBurnMarkArea.hpp"
#include "colorizer/SaberColorizer.hpp"

#include <algorithm>
#include <vector>

using namespace GlobalNamespace;
using namespace Chroma;

static std::vector<SaberBurnMarkArea*> burnMarkAreas;

static void OnSaberColorChanged_SaberBurnMarkArea(int saberType, SaberModelController*,
                                                Sombrero::FastColor const& color) {
  float h, s, v;
  Sombrero::FastColor::RGBToHSV(color, h, s, v);
  auto effectColor = Sombrero::FastColor::HSVToRGB(h, s, 1);
  for (auto& area : burnMarkAreas) {
    if (area && saberType >= 0 && saberType < area->_saberColors.size()) {
      area->_saberColors[saberType] = effectColor;
    }
  }
}

MAKE_HOOK_MATCH(SaberBurnMarkArea_Initialize, &SaberBurnMarkArea::Initialize, void, SaberBurnMarkArea* self,
                SettingsManager* settingsManager, SaberManager* saberManager, ColorManager* colorManager) {
  SaberBurnMarkArea_Initialize(self, settingsManager, saberManager, colorManager);
  if (!ChromaController::DoChromaHooks()) return;

  if (burnMarkAreas.empty()) {
    SaberColorizer::SaberColorChanged += &OnSaberColorChanged_SaberBurnMarkArea;
  }
  if (std::ranges::none_of(burnMarkAreas, [self](auto const& area) { return area == self; })) {
    burnMarkAreas.emplace_back(self);
  }
}

MAKE_HOOK_MATCH(SaberBurnMarkArea_OnDestroy, &SaberBurnMarkArea::OnDestroy, void, SaberBurnMarkArea* self) {
  std::erase_if(burnMarkAreas, [self](auto const& area) { return area == self; });
  if (burnMarkAreas.empty()) {
    SaberColorizer::SaberColorChanged -= &OnSaberColorChanged_SaberBurnMarkArea;
  }
  SaberBurnMarkArea_OnDestroy(self);
}

void SaberBurnMarkAreaHook() {
  INSTALL_HOOK(ChromaLogger::Logger, SaberBurnMarkArea_Initialize);
  INSTALL_HOOK(ChromaLogger::Logger, SaberBurnMarkArea_OnDestroy);
}
ChromaInstallHooks(SaberBurnMarkAreaHook)
