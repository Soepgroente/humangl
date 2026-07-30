#pragma once

#include <cstdint>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using i128 = __int128_t;

using ui8 = uint8_t;
using ui16 = uint16_t;
using ui32 = uint32_t;
using ui64 = uint64_t;
using ui128 = __uint128_t;

static_assert(sizeof(float) == 4);
static_assert(sizeof(double) == 8);

using f32 = float;
using f64 = double;
