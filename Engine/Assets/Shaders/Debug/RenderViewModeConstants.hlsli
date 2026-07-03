#pragma once

namespace ViewMode
{
	static const uint Lit = 0u;
	static const uint Wireframe = 1u;
	static const uint GBufferDiffuse = 2u;
	static const uint GBufferNormal = 3u;
	static const uint GBufferRoughness = 4u;
	static const uint GBufferMetallic = 5u;
	static const uint GBufferEmissive = 6u;
	static const uint GBufferAmbientOcclusion = 7u;
	static const uint GBufferSubsurfaceColor = 8u;
	static const uint GBufferSubsurfaceStrength = 9u;
	static const uint DirectDiffuse = 10u;
	static const uint DirectSpecular = 11u;
	static const uint DirectSubsurface = 12u;
	static const uint IndirectDiffuse = 13u;
	static const uint IndirectSpecular = 14u;
	static const uint IndirectSubsurface = 15u;
	static const uint InstanceGroups = 16u;
}
