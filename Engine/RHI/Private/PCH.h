#pragma once

#include "Config/RenderConfig.h"

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

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#include "third_party/D3DX12Includes.h"

#ifdef ENGINE_GPU_VALIDATION
	#include <d3d12sdklayers.h>
	#include <dxgidebug.h>
#endif

using Microsoft::WRL::ComPtr;
