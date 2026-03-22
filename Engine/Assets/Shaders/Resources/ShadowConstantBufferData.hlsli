#pragma once

struct ShadowConstantBufferData
{
	row_major float4x4 ViewProjMTX;
	float ShadowMapSize;
	float DepthBias;
	float NormalBias;
	float CascadeFarDepth;
};
