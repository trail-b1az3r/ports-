#pragma once

#include "HMUI/ViewController.hpp"


#include "custom-types/shared/macros.hpp"

#include "bsml/shared/macros.hpp"

DECLARE_CLASS_CODEGEN(Chroma, ModifierViewController, HMUI::ViewController) {

  DECLARE_INSTANCE_METHOD(void, DidActivate, bool first);
  DECLARE_OVERRIDE_METHOD_MATCH(void, DidActivate2, &HMUI::ViewController::DidActivate,
                          bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);

  DECLARE_BSML_PROPERTY(bool, environmentEnhancementsEnabled);
  DECLARE_BSML_PROPERTY(bool, customColorEventsEnabled);
  DECLARE_BSML_PROPERTY(bool, customNoteColors);

public:
  DECLARE_SIMPLE_DTOR();
};
