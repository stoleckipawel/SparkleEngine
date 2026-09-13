#pragma once

#include <cstdint>

enum class Visualization : std::uint32_t
{
	Lit = 0,
	Wireframe = 1,
	GBufferDiffuse = 2,
	GBufferNormal = 3,
	GBufferRoughness = 4,
	GBufferMetallic = 5,
	GBufferEmissive = 6,
	GBufferAmbientOcclusion = 7,
	GBufferSubsurfaceColor = 8,
	GBufferSubsurfaceStrength = 9,
	DirectDiffuse = 10,
	DirectSpecular = 11,
	DirectSubsurface = 12,
	IndirectDiffuse = 13,
	IndirectSpecular = 14,
	GpuSceneInstances = 15,
	Count = 16
};
