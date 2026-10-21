#pragma once

#include <cstddef>
#include <cstdlib>

inline void* av_mallocz(std::size_t size) {
    return std::calloc(1, size);
}
