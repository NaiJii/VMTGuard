#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <vector>
#include <iostream>
#include <format>
#include <unordered_map>

#define LOG(fmt, ...) std::cout << std::format(fmt, __VA_ARGS__) << "\n"
