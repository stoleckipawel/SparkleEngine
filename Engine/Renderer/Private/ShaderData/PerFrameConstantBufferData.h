#pragma once

#include "RenderConstantBufferValidation.h"

#include <DirectXMath.h>

#include <cstdint>

struct alignas(256) PerFrameConstantBufferData
{
	uint32_t FrameIndex;
	float TotalTime;
	float DeltaTime;
	float ScaledTotalTime;
	float ScaledDeltaTime;
	uint32_t ViewModeIndex;

	DirectX::XMFLOAT2 ViewportSize;
	DirectX::XMFLOAT2 ViewportSizeInv;
};
RHI_CBV_CHECK(PerFrameConstantBufferData);
