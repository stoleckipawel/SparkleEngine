#pragma once

#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "Viewport/ViewportContracts.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include <sl.h>

struct StreamlineViewConstantsInput final
{
	ViewCameraUniformData Camera = {};
	ViewTemporalUniformData Temporal = {};
	RenderViewportExtent RenderExtent = {};
	bool MotionVectorsCurrentMinusPrevious = true;
	bool ReversedDeviceDepth = true;
	bool ResetRequested = false;
};

void FillStreamlineViewConstants(sl::Constants& constants, const StreamlineViewConstantsInput& input);
sl::float4x4 ToStreamlineMatrix(const DirectX::XMFLOAT4X4& source);
#endif
