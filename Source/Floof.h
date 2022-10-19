#pragma once

#include <iostream>
#include <string>
#include <cassert>

namespace FLOOF {
#define LOG(msg) std::cout << msg
#ifndef NDEBUG
#define ASSERT(expr) assert(expr)
#else
#define ASSERT(expr)
#endif // !NDEBUG
}

