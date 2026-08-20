#pragma once

#include <cstdint>

struct RenderFrameIdentity final
{
	std::uint64_t FrameId = 0u;
	std::uint64_t ShaderPackageGeneration = 0u;
	std::uint64_t ImageProviderGeneration = 0u;
};
