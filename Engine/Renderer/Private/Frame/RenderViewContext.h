#pragma once

#include "RHI/Public/D3D12/Resources/D3D12ConstantBufferData.h"

#include <d3d12.h>

struct RenderViewContext
{
	PerViewConstantBufferData perViewData = {};
	D3D12_GPU_VIRTUAL_ADDRESS perViewGpuAddress = 0;
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissorRect = {};
};
