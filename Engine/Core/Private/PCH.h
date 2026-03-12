#pragma once

// ============================================================================
// Windows Configuration
// ============================================================================
#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

// ============================================================================
// C++ Standard Library - Commonly used across Core
// ============================================================================
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

// ============================================================================
// Windows - Required for platform abstraction
// ============================================================================
#include <Windows.h>

// ============================================================================
// Engine Logging - Available everywhere via PCH
// ============================================================================
#include "Diagnostics/Log.h"
