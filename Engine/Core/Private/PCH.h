#pragma once

#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

#include <Windows.h>

#include "Diagnostics/Log.h"
