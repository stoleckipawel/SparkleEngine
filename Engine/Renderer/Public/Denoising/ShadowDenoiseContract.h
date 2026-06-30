#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <cstdint>

namespace ShadowDenoiseContract
{
	inline constexpr PixelFormat PackedVisibilitySignalFormat = PixelFormat::R32G32B32A32_Float;
	inline constexpr PixelFormat DenoisedVisibilityFormat = PixelFormat::R32_Float;

	enum class ShadowDenoiseStage : std::uint8_t
	{
		Off,
		RawVisibility,
		DenoisedVisibility
	};

	struct ShadowDenoiseInputState
	{
		bool HasDepth = false;
		bool HasNormals = false;
		bool HasMotionVectors = false;
		bool HasJitter = false;
		bool HasHistory = false;
	};

	struct ShadowDenoiseTextures
	{
		FrameGraphTextureHandle PackedSignal;
		FrameGraphTextureHandle PackedSignalScratch;
		FrameGraphTextureHandle DenoisedVisibility;
		FrameGraphTextureHandle DenoisedVisibilityHistory;
	};

	struct ShadowDenoiseContract
	{
		ShadowDenoiseStage Stage = ShadowDenoiseStage::Off;
		bool UsesDenoiser = false;
		std::uint32_t RaysPerPixel = 1u;
		bool HasSceneTlas = true;
		ShadowDenoiseInputState Inputs{};
		ShadowDenoiseTextures Textures{};

		constexpr bool IsEnabled() const noexcept { return Stage != ShadowDenoiseStage::Off; }
		constexpr bool IsRawVisibilityOnly() const noexcept { return Stage == ShadowDenoiseStage::RawVisibility; }
		constexpr bool IsDenoisedPath() const noexcept { return Stage == ShadowDenoiseStage::DenoisedVisibility; }
	};

	struct BuildRequest
	{
		bool UseSoftShadows = false;
		bool RequestDenoiser = false;
		bool InlineRayQueryAvailable = false;
		bool HasSceneTlas = true;
		ShadowDenoiseInputState Inputs{};
	};

	ShadowDenoiseContract BuildContract(const BuildRequest& request) noexcept;
	const char* StageToString(ShadowDenoiseStage stage) noexcept;
	void LogContractSummary(const ShadowDenoiseContract& contract) noexcept;
}  // namespace ShadowDenoiseContract
