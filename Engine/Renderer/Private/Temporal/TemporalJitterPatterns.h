#pragma once

#include <DirectXMath.h>

#include <cstdint>

namespace TemporalJitterPatterns
{
enum class Pattern : uint8_t
{
	// Matches NVIDIA Donut temporal AA options.
	MSAA = 0,
	Halton,
	R2,
	WhiteNoise,
	// Internal/debug: no jitter.
	None
};

DirectX::XMFLOAT2 GeneratePatternSample(Pattern pattern, uint32_t frameIndex) noexcept;
DirectX::XMFLOAT2 GenerateJitterOffset(
    float width,
    float height,
    uint32_t frameIndex,
    Pattern pattern) noexcept;
}  // namespace TemporalJitterPatterns
