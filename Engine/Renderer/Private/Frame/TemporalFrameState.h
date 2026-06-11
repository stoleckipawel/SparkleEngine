#pragma once

#include "RHI/Public/Resources/RenderConstantBufferData.h"

struct RenderTemporalFrameState
{
	bool HasJitter = false;
	bool HasPreviousJitter = false;
	bool HistoryValid = false;
	DirectX::XMFLOAT2 JitterCurrent = {};
	DirectX::XMFLOAT2 JitterPrevious = {};
};

RenderTemporalFrameState BuildRenderTemporalFrameState(const PerTemporalConstantBufferData& temporalData) noexcept;

