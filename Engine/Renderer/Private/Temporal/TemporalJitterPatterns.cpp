#include "PCH.h"

#include "Temporal/TemporalJitterPatterns.h"

#include <random>

class TemporalJitterGeneration final
{
  public:
static constexpr float kNormalizedOffsetRange = 0.5f;
static constexpr uint32_t kHaltonBaseX = 2u;
static constexpr uint32_t kHaltonBaseY = 3u;
static constexpr float kR2G = 1.32471795724474602596f;
static constexpr float kR2A1 = 1.0f / kR2G;
static constexpr float kR2A2 = 1.0f / (kR2G * kR2G);

static float VanDerCorput(size_t base, size_t index) noexcept
{
	float result = 0.0f;
	float f = 1.0f;
	size_t i = index;
	while (i > 0u)
	{
		const size_t multiplier = i % base;
		f /= static_cast<float>(base);
		result += static_cast<float>(multiplier) * f;
		i /= base;
	}

	return result;
}

static DirectX::XMFLOAT2 CalculateMSAAJitter(uint32_t index) noexcept
{
	const DirectX::XMFLOAT2 offsets[] = {
		{0.0625f, -0.1875f},
		{-0.0625f, 0.1875f},
		{0.3125f, 0.0625f},
		{-0.1875f, -0.3125f},
		{-0.3125f, 0.3125f},
		{-0.4375f, -0.0625f},
		{0.1875f, 0.4375f},
		{0.4375f, -0.4375f}};
	return offsets[index % 8u];
}

static DirectX::XMFLOAT2 CalculateHaltonJitter(uint32_t index) noexcept
{
	return {
		VanDerCorput(kHaltonBaseX, index) - kNormalizedOffsetRange,
		VanDerCorput(kHaltonBaseY, index) - kNormalizedOffsetRange};
}

static DirectX::XMFLOAT2 CalculateR2Jitter(uint32_t index) noexcept
{
	const float jitterX = fmodf(static_cast<float>(index) * kR2A1, 1.0f);
	const float jitterY = fmodf(static_cast<float>(index) * kR2A2, 1.0f);
	return {jitterX - kNormalizedOffsetRange, jitterY - kNormalizedOffsetRange};
}

static DirectX::XMFLOAT2 CalculateWhiteNoiseJitter(uint32_t index) noexcept
{
	std::mt19937 rng(index);
	std::uniform_real_distribution<float> distribution(-kNormalizedOffsetRange, kNormalizedOffsetRange);
	return {distribution(rng), distribution(rng)};
}
};

DirectX::XMFLOAT2 TemporalJitterPatterns::GeneratePatternSample(Pattern pattern, uint32_t frameIndex) noexcept
{
	constexpr uint32_t kHaltonFrameWindow = 16u;

	switch (pattern)
	{
	case Pattern::MSAA:
		return TemporalJitterGeneration::CalculateMSAAJitter(frameIndex);
	case Pattern::Halton:
	{
		const uint32_t haltonFrameIndex = (frameIndex % kHaltonFrameWindow) + 1u;
		return TemporalJitterGeneration::CalculateHaltonJitter(haltonFrameIndex);
	}
	case Pattern::R2:
		return TemporalJitterGeneration::CalculateR2Jitter(frameIndex);
	case Pattern::WhiteNoise:
		return TemporalJitterGeneration::CalculateWhiteNoiseJitter(frameIndex);
	case Pattern::None:
		return {};
	default:
		return {};
	}
}

DirectX::XMFLOAT2 TemporalJitterPatterns::GenerateJitterOffset(
    float width,
    float height,
    uint32_t frameIndex,
    Pattern pattern) noexcept
{
	if (width <= 1.0f || height <= 1.0f)
	{
		return {};
	}

	const DirectX::XMFLOAT2 patternOffset = GeneratePatternSample(pattern, frameIndex);
	const float x = (2.0f * patternOffset.x) / width;
	const float y = (2.0f * patternOffset.y) / height;
	return {x, -y};
}
