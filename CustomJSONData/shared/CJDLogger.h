#pragma once
#include "beatsaber-hook/shared/utils.hpp"
#include "beatsaber-hook/shared/exceptions.hpp"
#include "beatsaber-hook/shared/hooking.hpp"
#include "beatsaber-hook/shared/safeptr.hpp"
#include "beatsaber-hook/shared/callback.hpp"
#include "beatsaber-hook/shared/listw.hpp"
#include "beatsaber-hook/shared/arrayw.hpp"
#include "beatsaber-hook/shared/stringw.hpp"

#include "scotland2/shared/loader.hpp"

#include "paper2_scotland2/shared/logger.hpp"

using LogLevel = Paper::LogLevel;

namespace CustomJSONData {
static modloader::ModInfo const modInfo{"CustomJSONData", VERSION, 1};
}

class CJDLogger {
public:
  // Register file log in main.cpp
  static constexpr auto Logger =
      Paper::ConstLoggerContext("CustomJSONData"); // Paper::Logger::WithContext<"CustomJSONData", false>();
};

// Implements a try-catch handler which will first attempt to run the provided body.
// If there is an uncaught RunMethodException, it will first attempt to log the backtrace.
// If it holds a valid C# exception, it will attempt to raise it, such that it is caught in the il2cpp domain.
// If an exception is thrown that is otherwise what-able is caught, it will attempt to call the what() method
// and then rethrow the exception to the il2cpp domain.
// If an unknown exception is caught, it will terminate explicitly, as opposed to letting it be thrown across the il2cpp
// domain. All logs that occur as a result of this function will be under the core beatsaber-hook global logger.
#define PAPER_IL2CPP_CATCH_HANDLER(...)                                                                                \
  try {                                                                                                                \
    __VA_ARGS__                                                                                                        \
  } catch (::i2c::trace_exception const& exc) {                                                                        \
    CJDLogger::Logger.error("Uncaught trace_exception! what(): {}", exc.what());                                       \
    Paper::Logger::WaitForFlush();                                                                                     \
    exc.log_backtrace();                                                                                               \
    CJDLogger::Logger.Backtrace(100);                                                                                  \
    SAFE_ABORT();                                                                                                      \
  } catch (::std::exception const& exc) {                                                                              \
    CJDLogger::Logger.error("Uncaught C++ exception! type name: {}, what(): {}", typeid(exc).name(), exc.what());      \
    Paper::Logger::WaitForFlush();                                                                                     \
    CJDLogger::Logger.Backtrace(100);                                                                                  \
    SAFE_ABORT();                                                                                                      \
  } catch (...) {                                                                                                      \
    CJDLogger::Logger.error("Uncaught, unknown C++ exception (not std::exception) with no known what() method!");      \
    Paper::Logger::WaitForFlush();                                                                                     \
    CJDLogger::Logger.Backtrace(100);                                                                                  \
    SAFE_ABORT();                                                                                                      \
  }

template <auto Func, class T>
/// @brief Exposes a static wrapper method that forwards to the provided function pointer, wrapping it in
/// IL2CPP_CATCH_HANDLER.
struct PaperHookCatchWrapper;

template <auto Func, class R, class... TArgs> struct PaperHookCatchWrapper<Func, R (*)(TArgs...)> {
  static R wrapper(TArgs... args) {
    PAPER_IL2CPP_CATCH_HANDLER(return Func(args...);)
  }
};

// Make a hook that uses the provided method pointer, ensuring the signature matches (see MAKE_HOOK_MATCH),
// and wraps the hook body in PAPER_IL2CPP_CATCH_HANDLER. Installed with INSTALL_HOOK / INSTALL_HOOK_ORIG.
#define MAKE_PAPER_HOOK_MATCH(name_, method, retval, ...)                                                              \
  struct BS_HOOK_HIDDEN hook_##name_ {                                                                                 \
    static constexpr auto cast_test = []<typename T>() { return requires { static_cast<T>(method); }; };               \
    using func_t = retval (*)(__VA_ARGS__);                                                                            \
    using cast_t = ::i2c::detail::method_check<cast_test, func_t>::type;                                              \
    static_assert(cast_test.operator()<cast_t>(), "Hook method signature does not match!");                            \
    static_assert(::i2c::detail::match_hookable<static_cast<cast_t>(method)>, "Method cannot be hooked!");            \
    constexpr static char const* name() {                                                                              \
      return #name_;                                                                                                   \
    }                                                                                                                  \
    static auto addr() {                                                                                               \
      return ::i2c::metadata_getter<static_cast<cast_t>(method)>::method_info();                                      \
    }                                                                                                                  \
    static func_t hook() {                                                                                             \
      return &::PaperHookCatchWrapper<&hook_m_##name_, func_t>::wrapper;                                               \
    }                                                                                                                  \
    static func_t* trampoline() {                                                                                      \
      return &name_;                                                                                                   \
    }                                                                                                                  \
    static inline flamingo::HookHandle install_handle;                                                                 \
    static inline flamingo::HookPriority install_priority;                                                             \
    static retval hook_m_##name_(__VA_ARGS__);                                                                         \
    static inline retval (*name_)(__VA_ARGS__) = nullptr;                                                              \
  };                                                                                                                   \
  retval hook_##name_::hook_m_##name_(__VA_ARGS__)
