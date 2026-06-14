#pragma once

#include "RenderConstantBufferValidation.h"

#include <DirectXMath.h>

#include <cstdint>

struct alignas(256) PerTemporalConstantBufferData
{
	DirectX::XMFLOAT4X4 PrevViewMTX = {};
	DirectX::XMFLOAT4X4 PrevProjectionMTX = {};
	DirectX::XMFLOAT4X4 PrevViewProjMTX = {};
	DirectX::XMFLOAT2 JitterCurrent = {0.0f, 0.0f};
	DirectX::XMFLOAT2 JitterPrevious = {0.0f, 0.0f};
	uint32_t HistoryValid = 0;
	DirectX::XMFLOAT4 _pad0 = {};
	DirectX::XMFLOAT4 _pad1 = {};
};
RHI_CBV_CHECK(PerTemporalConstantBufferData);
