#pragma once

namespace RenderViewMode
{
	static const uint Lit = 0u;
	static const uint ReferencePathTracer = 1u;
	static const uint Wireframe = 2u;
	static const uint GBufferDiffuse = 3u;
	static const uint GBufferWorldNormal = 4u;
	static const uint GBufferWorldTangent = 5u;
	static const uint GBufferRoughness = 6u;
	static const uint GBufferMetallic = 7u;
	static const uint GBufferEmissive = 8u;
	static const uint GBufferAmbientOcclusion = 9u;
	static const uint GBufferSubsurfaceColor = 10u;
	static const uint GBufferSubsurfaceStrength = 11u;
	static const uint DirectDiffuse = 12u;
	static const uint DirectSpecular = 13u;
	static const uint DirectSubsurface = 14u;
	static const uint IndirectDiffuse = 15u;
	static const uint IndirectSpecular = 16u;
	static const uint GpuSceneInstances = 17u;
}
