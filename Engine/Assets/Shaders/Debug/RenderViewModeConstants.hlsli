#pragma once

namespace ViewMode
{
	static const uint Lit = 0u;
	static const uint ReferencePathTracer = 1u;
	static const uint Wireframe = 2u;
	static const uint GBufferDiffuse = 3u;
	static const uint GBufferNormal = 4u;
	static const uint GBufferRoughness = 5u;
	static const uint GBufferMetallic = 6u;
	static const uint GBufferEmissive = 7u;
	static const uint GBufferAmbientOcclusion = 8u;
	static const uint GBufferSubsurfaceColor = 9u;
	static const uint GBufferSubsurfaceStrength = 10u;
	static const uint DirectDiffuse = 11u;
	static const uint DirectSpecular = 12u;
	static const uint DirectSubsurface = 13u;
	static const uint IndirectDiffuse = 14u;
	static const uint IndirectSpecular = 15u;
	static const uint GpuSceneInstances = 16u;
}
