#pragma once

#include "ShaderData/PerTemporalConstantBufferData.h"

#include <DirectXMath.h>

struct RenderTemporalFrameState
{
	bool HasJitter = false;
	bool HasPreviousJitter = false;
	bool HistoryValid = false;
	DirectX::XMFLOAT2 CurrentJitterNdc = {};
	DirectX::XMFLOAT2 PreviousJitterNdc = {};
};

RenderTemporalFrameState BuildRenderTemporalFrameState(const PerTemporalConstantBufferData& temporalData) noexcept;
