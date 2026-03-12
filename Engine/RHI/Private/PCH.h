#pragma once

#include "RHIConfig.h"

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

#include "Log.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include <d3dx12.h>

#ifdef ENGINE_GPU_VALIDATION
	#include <d3d12sdklayers.h>
	#include <dxgidebug.h>
#endif

using Microsoft::WRL::ComPtr;
