#pragma once

#if ENABLE_LOGGING
#include "Log.h"
#include "UtilityMacros.h"
#include <DebugBreak.h>
#include <imgui/imgui.h>

inline float GetTimeDifferenceMs(std::chrono::steady_clock::time_point from) {
  const auto currentTime = std::chrono::steady_clock::now();
  auto timeDifference =
      std::chrono::duration_cast<std::chrono::microseconds>(currentTime - from);
  return static_cast<float>(timeDifference.count()) / 1000.0f;
}

class Timer {
  bool m_Deleted = false;

public:
  const std::string Name;
  std::chrono::steady_clock::time_point Begin;

  explicit Timer(const std::string &name) : Name(name) {
    if (name.empty())
      debug_break();
    Begin = std::chrono::steady_clock::now();
  }

  void Reset() noexcept {
    Begin = std::chrono::steady_clock::now();
    m_Deleted = false;
  }

  ~Timer() {
    if (m_Deleted)
      return;

    NG_TRACE("{} took {:.1f} ms", Name,
             static_cast<float>(
                 std::chrono::duration_cast<std::chrono::microseconds>(
                     std::chrono::steady_clock::now() - Begin)
                     .count()) /
                 1000.0f);
    m_Deleted = true;
  }
};

class Profiler {
  bool m_Deleted = false;

public:
  std::string Name;
  std::chrono::steady_clock::time_point Begin;

  Profiler(const std::string &name) : Name(name) {
    if (name.empty())
      debug_break();
    Begin = std::chrono::steady_clock::now();
  }

  void Reset() noexcept {
    Begin = std::chrono::steady_clock::now();
    m_Deleted = false;
  }

  ~Profiler() {
    if (m_Deleted)
      return;

    ImGui::Text("%s took %.2f ms", Name.c_str(),
                static_cast<float>(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - Begin)
                        .count()) /
                    1000.0f);
    m_Deleted = true;
  }
};

// Code modified from Hazel:
// https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Hazel/Debug/Instrumentor.h
template <size_t N> struct ChangeResult {
  char Data[N];
};

template <size_t N, size_t K, size_t L>
constexpr ChangeResult<N> CleanupOutputString(const char (&expr)[N],
                                              const char (&remove1)[K],
                                              const char (&remove2)[L]) {
  ChangeResult<N> result = {};

  size_t srcIndex = 0;
  size_t dstIndex = 0;
  while (srcIndex < N) {
    size_t matchIndex = 0;
    while (matchIndex < K - 1 && srcIndex + matchIndex < N - 1 &&
           expr[srcIndex + matchIndex] == remove1[matchIndex])
      matchIndex++;
    if (matchIndex == K - 1)
      srcIndex += matchIndex;

    matchIndex = 0;
    while (matchIndex < L - 1 && srcIndex + matchIndex < N - 1 &&
           expr[srcIndex + matchIndex] == remove2[matchIndex])
      matchIndex++;

    if (matchIndex == L - 1)
      srcIndex += matchIndex;

    result.Data[dstIndex++] = expr[srcIndex] == '"' ? '\'' : expr[srcIndex];
    srcIndex++;
  }
  return result;
}

// Resolve which function signature macro will be used. Note that this only
// is resolved when the (pre)compiler starts, so the syntax highlighting
// could mark the wrong one in your editor!
#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) ||    \
    (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
#define FUNC_SIG __PRETTY_FUNCTION__
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define FUNC_SIG __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) ||              \
    (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
#define FUNC_SIG __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
#define FUNC_SIG __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#define FUNC_SIG __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
#define FUNC_SIG __func__
#else
#define FUNC_SIG "FUNC_SIG unknown!"
#endif

#define _PROFILER_SCOPE(name, line)                                            \
  constexpr auto fixedName##line =                                             \
      CleanupOutputString(name, "__cdecl ", "void ");                          \
  Profiler p##line(fixedName##line.Data)
#define PROFILE_SCOPE_MAINLOOP(name) _PROFILER_SCOPE(name, __LINE__)
#define _TIMER_SCOPE(name, line)                                               \
  constexpr auto fixedName##line =                                             \
      CleanupOutputString(name, "__cdecl ", "void ");                          \
  Timer t##line(fixedName##line.Data)
#define PROFILE_SCOPE_ONCE(name) _TIMER_SCOPE(name, __LINE__)
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATION
#define PROFILE_FUNCTION_ONCE() PROFILE_SCOPE_ONCE(FUNC_SIG)
DISABLE_WARNING_POP
#else
// Make existing code work, will (hopefully) get optimized away by the compiler
struct Timer {
  static constexpr std::string Name = "";
  static constexpr std::chrono::steady_clock::time_point Begin =
      std::chrono::steady_clock::time_point::max();

  Timer([[maybe_unused]] const std::string &name) {}
  void Reset() {}
};
struct Profiler {
  static constexpr std::string Name = "";
  static constexpr std::chrono::steady_clock::time_point Begin =
      std::chrono::steady_clock::time_point::max();

  Profiler([[maybe_unused]] const std::string &name) {}
  void Reset() {}
};
#define PROFILE_SCOPE_MAINLOOP(name)
#define PROFILE_SCOPE_ONCE(name)
#define PROFILE_SCOPE_ONCE(name)
#define PROFILE_FUNCTION_ONCE()
#endif
