#pragma once

namespace Shadow
{
	uint GetShadowMapIndex(uint lightIndex, uint cascadeIndex)
	{
		return lightIndex * MAX_SHADOW_CASCADES + cascadeIndex;
	}

	ShadowConstantBufferData GetDirectionalShadowCascade(uint lightIndex, uint cascadeIndex)
	{
		return ViewLighting.Shadows[GetShadowMapIndex(lightIndex, cascadeIndex)];
	}

	float GetCascadeBlendWeight(float cameraDistance, float splitDepth)
	{
		const float blendRange = max(0.5f, splitDepth * 0.05f);
		const float blendStart = max(0.0f, splitDepth - blendRange);
		return saturate((cameraDistance - blendStart) / blendRange);
	}
}  // namespace Shadow