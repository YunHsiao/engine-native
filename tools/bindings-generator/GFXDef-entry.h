#include <type_traits>
#include <vector>
#include <string>

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;

typedef unsigned int FlagBits;

#define CC_ENUM_OPERATORS(type_)                                                                                                                                                             \
    inline type_ operator|(type_ lhs, type_ rhs) { return (type_)(static_cast<std::underlying_type<type_>::type /**/>(lhs) | static_cast<std::underlying_type<type_>::type /**/>(rhs)); } \
    inline void operator|=(type_ &lhs, type_ rhs) { lhs = (type_)(static_cast<std::underlying_type<type_>::type /**/>(lhs) | static_cast<std::underlying_type<type_>::type /**/>(rhs)); } \
    inline int operator&(type_ lhs, type_ rhs) { return (int)(static_cast<std::underlying_type<type_>::type /**/>(lhs) & static_cast<std::underlying_type<type_>::type /**/>(rhs)); }     \
    inline void operator&=(type_ &lhs, type_ rhs) { lhs = (type_)(static_cast<std::underlying_type<type_>::type /**/>(lhs) & static_cast<std::underlying_type<type_>::type /**/>(rhs)); } \
    inline bool operator||(type_ lhs, type_ rhs) { return (static_cast<std::underlying_type<type_>::type /**/>(lhs) || static_cast<std::underlying_type<type_>::type /**/>(rhs)); }       \
    inline bool operator&&(type_ lhs, type_ rhs) { return (static_cast<std::underlying_type<type_>::type /**/>(lhs) && static_cast<std::underlying_type<type_>::type /**/>(rhs)); }

namespace cc {

template <typename T>
using vector = std::vector<T>;

typedef std::string String;

namespace math {

template <typename T>
inline T Abs(T x) {
    return x > 0 ? x : -x;
}

template <typename T>
inline T Sgn(T x) {
    return (x < T(0) ? T(-1) : (x > T(0) ? T(1) : T(0)));
}

template <typename T>
inline T Sqr(T x) {
    return x * x;
}

template <typename T>
inline bool IsPowerOfTwo(T n) {
    return (n & (n - 1)) == 0;
}

inline bool IsEqualF(float lhs, float rhs, float precision = 0.000001f) {
    return (Abs<float>(lhs - rhs) < precision);
}

inline bool IsNotEqualF(float lhs, float rhs, float precision = 0.000001f) {
    return (Abs<float>(lhs - rhs) > precision);
}

} // namespace math
} // namespace cc

#include "../../cocos/renderer/core/gfx/GFXDef.h"
