#pragma once

// Utility macros

#define SWAP(name) std::swap(name, other.name)

#define MOVE_CONSTRUCT(name) name(std::move(other.name))

#define OVERLOAD_OPERATOR_RVALUE(classname)                                    \
  classname &operator=(classname &&other) noexcept {                           \
    swap(other);                                                               \
    return *this;                                                              \
  }

#define OVERLOAD_STD_SWAP(classname)                                           \
  namespace std {                                                              \
  inline void swap(classname &a, classname &b) noexcept(noexcept(a.swap(b))) { \
    a.swap(b);                                                                 \
  }                                                                            \
  }

#define DELETE_COPY_CONSTRUCTOR(classname)                                     \
  classname(const classname &other) = delete;                                  \
  classname &operator=(const classname &other) = delete;

#if defined(__GNUC__) || defined(__clang__)
#define DO_PRAGMA(X) _Pragma(#X);
#define DISABLE_WARNING_PUSH DO_PRAGMA(GCC diagnostic push)
#define DISABLE_WARNING_POP DO_PRAGMA(GCC diagnostic pop)
#define DISABLE_WARNING(warningname)                                           \
  DO_PRAGMA(GCC diagnostic ignored #warningname)
#define DISABLE_WARNING_STRING(warningname)                                    \
  DO_PRAGMA(GCC diagnostic ignored warningname)
#define DISABLE_ALL_WARNINGS                                                   \
  DISABLE_WARNING_PUSH                                                         \
  DISABLE_WARNING(-Wall)                                                       \
  DISABLE_WARNING(-Wextra)                                                     \
  DISABLE_WARNING(-Wconversion) DISABLE_WARNING(-Wpedantic)

#define DISABLE_WARNING_LANGUAGE_EXTENSION                                     \
  DISABLE_WARNING_STRING("-Wlanguage-extension-token")
#define DISABLE_WARNING_DEPRECATION                                            \
  DISABLE_WARNING_STRING("-Wdeprecated-declarations")
#define SUPPRESS_DEPRECATION_WARNING

#elif defined(_MSC_VER)

#define DISABLE_WARNING_PUSH __pragma(warning(push))
#define DISABLE_WARNING_POP __pragma(warning(pop))
#define DISABLE_WARNING(warningNumber)                                         \
  __pragma(warning(disable : warningNumber))
#define DISABLE_ALL_WARNINGS __pragma(warning(push, 0))
#define SUPPRESS_WARNING_ONE_LINE(warningNumber)                               \
  __pragma(warning(suppress : warningNumber))
#define SUPPRESS_DEPRECATION_WARNING SUPPRESS_WARNING_ONE_LINE(4996)
#define DISABLE_WARNING_DEPRECATION DISABLE_WARNING(4996)
#define DISABLE_WARNING_LANGUAGE_EXTENSION

#else

#define DISABLE_WARNING_PUSH
#define DISABLE_WARNING_POP
#define DISABLE_WARNING_LANGUAGE_EXTENSION
#define DISABLE_ALL_WARNINGS

#endif
