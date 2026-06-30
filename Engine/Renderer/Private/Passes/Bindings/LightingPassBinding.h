#pragma once

#include "Frame/Core/FrameContext.h"

namespace LightingPassBinding
{
	template <typename TParameterInstance>
	void SetParameters(TParameterInstance& parameters, const FrameContext& frame) noexcept
	{
		parameters->ViewLighting = frame.lighting.GetConstants();
		parameters->DirectionalLights = frame.lighting.GetDirectionalLightsShaderResourceView();
		parameters->PointLights = frame.lighting.GetPointLightsShaderResourceView();
		parameters->SpotLights = frame.lighting.GetSpotLightsShaderResourceView();
		parameters->RectLights = frame.lighting.GetRectLightsShaderResourceView();
	}
}
