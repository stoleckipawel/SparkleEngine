#pragma once

#define NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <array>
#include <algorithm>

#include <Windows.h>
#include <wrl/client.h>
#include <DirectXMath.h>

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"
