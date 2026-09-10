#pragma once

#include <cstdint>

enum class RenderViewMode : std::uint32_t
{
	Lit = 0,
	ReferencePathTracer = 1,
	Wireframe = 2,
	GBufferDiffuse = 3,
	GBufferNormal = 4,
	GBufferRoughness = 5,
	GBufferMetallic = 6,
	GBufferEmissive = 7,
	GBufferAmbientOcclusion = 8,
	GBufferSubsurfaceColor = 9,
	GBufferSubsurfaceStrength = 10,
	DirectDiffuse = 11,
	DirectSpecular = 12,
	DirectSubsurface = 13,
	IndirectDiffuse = 14,
	IndirectSpecular = 15,
	GpuSceneInstances = 16,
	Count = 17
};
