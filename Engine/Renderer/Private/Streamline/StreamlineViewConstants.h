#pragma once

#include "Frame/Temporal/TemporalFrameState.h"
#include "ShaderData/PerFrameConstantBufferData.h"
#include "ShaderData/RenderViewCameraData.h"
#include "Viewport/ViewportContracts.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

struct StreamlineViewConstantsInput final
{
	PerViewCameraConstantBufferData Camera = {};
	PerTemporalConstantBufferData TemporalData = {};
	RenderTemporalFrameState TemporalState = {};
	RenderViewportExtent RenderExtent = {};
	bool MotionVectorsCurrentMinusPrevious = true;
	bool ReversedDeviceDepth = true;
	bool ResetRequested = false;
};

void FillStreamlineViewConstants(sl::Constants& constants, const StreamlineViewConstantsInput& input) noexcept;
sl::float4x4 ToStreamlineMatrix(const DirectX::XMFLOAT4X4& source) noexcept;
#endif
