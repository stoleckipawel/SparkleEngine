#pragma once

#include "PCH.h"

#include <d3d12.h>
#include <dxgi1_6.h>

#include "D3D12/ThirdParty/D3DX12Includes.h"

#ifdef ENGINE_GPU_VALIDATION
  #include <d3d12sdklayers.h>
  #include <dxgidebug.h>
#endif

using Microsoft::WRL::ComPtr;
