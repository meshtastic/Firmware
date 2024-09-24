#pragma once
#include "DebugConfiguration.h"
#include <stdint.h>

/// C++ v17+ clamp function, limits a given value to a range defined by lo and hi
template <class T> constexpr const T &clamp(const T &v, const T &lo, const T &hi)
{
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

#if (defined(ARCH_PORTDUINO) && !defined(STRNSTR))
#define STRNSTR
#include <string.h>
char *strnstr(const char *s, const char *find, size_t slen);
#endif

void printBytes(const char *label, const uint8_t *p, size_t numbytes);

// is the memory region filled with a single character?
bool memfll(const uint8_t *mem, uint8_t find, size_t numbytes);

#define IS_ONE_OF(item, ...) (std::find(std::begin({__VA_ARGS__}), std::end({__VA_ARGS__}), item) != std::end({__VA_ARGS__}))