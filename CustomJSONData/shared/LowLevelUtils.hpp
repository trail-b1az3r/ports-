#pragma once

#include "beatsaber-hook/shared/utils.hpp"

/// Collection of methods that are intended to be low level performance optimizations for
/// niche use cases
/// Might PR to bs-hooks sometime later
namespace CustomJSONData {

/// Replacement for the removed CustomJSONData::ClassStandardName: "Namespace::Name"
inline std::string ClassStandardName(Il2CppClass const* klass, bool = false) {
  if (!klass) return "null";
  if (!klass->namespaze || klass->namespaze[0] == '\0') return klass->name;
  return fmt::format("{}::{}", klass->namespaze, klass->name);
}
inline std::string ClassStandardName(Il2CppObject const* obj, bool = false) {
  return obj ? ClassStandardName(obj->klass) : "null";
}

template <class T, class... TArgs>
concept CtorArgs = requires(T t, TArgs&&... args) {
  { T::New_ctor(std::forward<TArgs>(args)...) };
};

template <class T, class... TArgs>
requires(CtorArgs<std::remove_pointer_t<T>, TArgs...>)
constexpr T NewFast(TArgs&&... args) {
  return std::remove_pointer_t<T>::New_ctor(std::forward<TArgs>(args)...);
}

// // Faster allocation with method cache
// template <class T, class... TArgs>
// // Bonus points for a requires clause here for i2c::class_of<T>()
// T NewFastKlass(Il2CppClass* klass, TArgs&&... args) {
//   //        return CRASH_UNLESS(il2cpp_utils::New<T, il2cpp_utils::CreationType::Temporary, TArgs...>(klass,
//   //        std::forward<TArgs>(args)...));
//   static auto ctor = CRASH_UNLESS(il2cpp_utils::FindMethod(
//       klass, ".ctor", std::array<Il2CppType const*, sizeof...(TArgs)>{ il2cpp_utils::ExtractIndependentType<TArgs>()... }));
//   auto* obj = CRASH_UNLESS(i2c::functions::object_new(klass));
//   il2cpp_utils::RunMethodRethrow(obj, ctor, std::forward<TArgs>(args)...);
//   return reinterpret_cast<T>(obj);
// }

// // Faster allocation with method cache
// template <class T, class... TArgs>
// // Bonus points for a requires clause here for i2c::class_of<T>()
// T NewFastUnsafe(TArgs&&... args) {
//   static auto klass = i2c::class_of<T>();
//   return NewFastKlass<T, TArgs...>(klass, std::forward<TArgs>(args)...);
// }

// // Faster allocation with method cache
// template <class T, class... TArgs>
// requires(CtorArgs<std::remove_pointer_t<T>, TArgs...>)
//     // Bonus points for a requires clause here for i2c::class_of<T>()
//     T NewFast(TArgs&&... args) {
//   static auto klass = i2c::class_of<T>();
//   return NewFastKlass<T, TArgs...>(klass, std::forward<TArgs>(args)...);
// }

// Declare a method with name that will be called on construction.
//#define DECLARE_FASTER_CTOR(name, ...)  DECLARE_CTOR(name, __VA_ARGS__)

// custom-types 0.21 New_ctor already allocates directly with object_new, so this is now an alias.
#define DECLARE_FASTER_CTOR(name, ...) DECLARE_CTOR(name __VA_OPT__(,) __VA_ARGS__)
} // namespace CustomJSONData