#pragma once

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
