#pragma once

#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#include <cstdint>
#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <algorithm>

#include <Windows.h>

#include "Diagnostics/Log.h"
