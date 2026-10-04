#pragma once

#include <cstdint>

enum class RenderViewMode : std::uint32_t
{
	Lit = 0,
	ReferencePathTracer = 1,
	Wireframe = 2,
	GBufferDiffuse = 3,
	GBufferWorldNormal = 4,
	GBufferWorldTangent = 5,
	GBufferRoughness = 6,
	GBufferMetallic = 7,
	GBufferEmissive = 8,
	GBufferAmbientOcclusion = 9,
	GBufferSubsurfaceColor = 10,
	GBufferSubsurfaceStrength = 11,
	DirectDiffuse = 12,
	DirectSpecular = 13,
	DirectSubsurface = 14,
	IndirectDiffuse = 15,
	IndirectSpecular = 16,
	GpuSceneInstances = 17,
	Count = 18
};
