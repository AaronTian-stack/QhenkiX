#pragma once

#if defined(__GNUC__) || defined(__clang__)
#include <algorithm>
#include <iterator>
#include <utility>
#if __has_include(<format>)
#include <format>
#endif
#endif

// clang-format off
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXPackedVector.h>
// clang-format on
