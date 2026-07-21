#pragma once

#include <cstdint>

enum class RenderMotionVectorConvention : std::uint8_t { CurrentToPreviousPixels };
enum class RenderDepthConvention : std::uint8_t { ReversedZZeroToOne };

struct RenderFrameMetadata final
{
	std::uint64_t FrameId = 0;
	std::uint64_t FrameGeneration = 0;
	std::uint64_t ProviderGeneration = 0;
	std::uint32_t JitterSampleIndex = 0;
	std::uint32_t RenderWidth = 0;
	std::uint32_t RenderHeight = 0;
	std::uint32_t OutputWidth = 0;
	std::uint32_t OutputHeight = 0;
	float Exposure = 1.0f;
	RenderMotionVectorConvention MotionVectors = RenderMotionVectorConvention::CurrentToPreviousPixels;
	RenderDepthConvention Depth = RenderDepthConvention::ReversedZZeroToOne;
	bool CameraCut = false;
	bool CameraTeleported = false;
	bool ResetHistory = false;
};
