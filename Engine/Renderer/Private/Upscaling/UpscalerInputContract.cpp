#include "../PCH.h"
#include "Upscaling/UpscalerInputContract.h"

#include <cstddef>
#include <format>
#include <string_view>
#include <utility>

namespace
{
	void AddMissing(std::vector<std::string>& missing, bool present, std::string_view label)
	{
		if (!present)
		{
			missing.emplace_back(label);
		}
	}

	std::string JoinRequirements(const std::vector<std::string>& requirements)
	{
		std::string result;
		for (std::size_t index = 0; index < requirements.size(); ++index)
		{
			if (index > 0)
			{
				result += ", ";
			}
			result += requirements[index];
		}
		return result;
	}
}

UpscalerInputContractValidation ValidateUpscalerInputContract(const UpscalerInputContract& contract)
{
	std::vector<std::string> missing;
	AddMissing(missing, static_cast<bool>(contract.HudlessSceneColor), "HUD-less scene color");
	AddMissing(missing, static_cast<bool>(contract.Depth), "depth");
	AddMissing(missing, static_cast<bool>(contract.MotionVectors), "motion vectors");
	AddMissing(missing, static_cast<bool>(contract.FinalOutput), "final output target");
	AddMissing(missing, contract.RenderExtent.IsValid(), "valid render extent");
	AddMissing(missing, contract.OutputExtent.IsValid(), "valid output extent");
	AddMissing(missing, contract.MotionVectorConvention.Units != EUpscalerMotionVectorUnits::Unknown, "motion-vector units");
	AddMissing(missing, contract.MotionVectorConvention.Direction != EUpscalerMotionVectorDirection::Unknown, "motion-vector direction");
	AddMissing(missing, contract.MotionVectorConvention.JitterMode != EUpscalerMotionVectorJitterMode::Unknown, "motion-vector jitter convention");
	AddMissing(missing, contract.DepthConvention != EUpscalerDepthConvention::Unknown, "depth convention");
	if (contract.ExposureRequired)
	{
		AddMissing(missing, static_cast<bool>(contract.Exposure), "exposure texture");
	}

	const bool valid = missing.empty();
	return UpscalerInputContractValidation{
	    .Valid = valid,
	    .MissingRequirements = std::move(missing),
	    .Summary = valid
	                   ? std::format(
	                         "valid renderExtent={}x{} outputExtent={}x{} motionVectors={} {} {} depth={} historyValid={} resetRequested={}",
	                         contract.RenderExtent.Width,
	                         contract.RenderExtent.Height,
	                         contract.OutputExtent.Width,
	                         contract.OutputExtent.Height,
	                         UpscalerMotionVectorUnitsToString(contract.MotionVectorConvention.Units),
	                         UpscalerMotionVectorDirectionToString(contract.MotionVectorConvention.Direction),
	                         UpscalerMotionVectorJitterModeToString(contract.MotionVectorConvention.JitterMode),
	                         UpscalerDepthConventionToString(contract.DepthConvention),
	                         contract.TemporalState.HistoryValid ? "true" : "false",
	                         contract.ResetRequested ? "true" : "false")
	                   : std::format("invalid missing={}", JoinRequirements(missing))};
}

const char* UpscalerMotionVectorUnitsToString(EUpscalerMotionVectorUnits units) noexcept
{
	switch (units)
	{
		case EUpscalerMotionVectorUnits::PixelDelta:
			return "PixelDelta";
		case EUpscalerMotionVectorUnits::Unknown:
		default:
			return "Unknown";
	}
}

const char* UpscalerMotionVectorDirectionToString(EUpscalerMotionVectorDirection direction) noexcept
{
	switch (direction)
	{
		case EUpscalerMotionVectorDirection::CurrentMinusPrevious:
			return "CurrentMinusPrevious";
		case EUpscalerMotionVectorDirection::Unknown:
		default:
			return "Unknown";
	}
}

const char* UpscalerMotionVectorJitterModeToString(EUpscalerMotionVectorJitterMode jitterMode) noexcept
{
	switch (jitterMode)
	{
		case EUpscalerMotionVectorJitterMode::CurrentAndPreviousClipPositionsAreJittered:
			return "CurrentAndPreviousClipPositionsAreJittered";
		case EUpscalerMotionVectorJitterMode::Unknown:
		default:
			return "Unknown";
	}
}

const char* UpscalerDepthConventionToString(EUpscalerDepthConvention convention) noexcept
{
	switch (convention)
	{
		case EUpscalerDepthConvention::DeviceDepth:
			return "DeviceDepth";
		case EUpscalerDepthConvention::Unknown:
		default:
			return "Unknown";
	}
}
