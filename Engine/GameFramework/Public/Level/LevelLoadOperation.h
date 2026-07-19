#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>

enum class LevelLoadOperationStage : std::uint8_t
{
	Idle,
	Reading,
	Decoding,
	Validating,
	Ready,
	Failed,
	Cancelled
};

struct SPARKLE_ENGINE_API LevelLoadOperationProgress final
{
	std::uint64_t RequestId = 0;
	LevelLoadOperationStage Stage = LevelLoadOperationStage::Idle;
	std::uint32_t CompletedAssets = 0;
	std::uint32_t TotalAssets = 0;
};
