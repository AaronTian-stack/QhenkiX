#pragma once

#if defined(__GNUC__) || defined(__clang__)
#include <algorithm>
#include <iterator>
#include <utility>
#if __has_include(<format>)
#include <format>
#endif
#endif

// Stop SAL annotation `__valid` from colliding with libstdc++
#if defined(__valid)
#pragma push_macro("__valid")
#define HAS_PREEXISTING_VALID_MACRO 1
#endif

// clang-format off
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
// clang-format on

#if defined(HAS_PREEXISTING_VALID_MACRO)
#pragma pop_macro("__valid")
#undef HAS_PREEXISTING_VALID_MACRO
#elif defined(__valid)
#undef __valid
#endif
