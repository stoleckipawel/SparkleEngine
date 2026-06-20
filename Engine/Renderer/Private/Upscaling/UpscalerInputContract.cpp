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

RendererProviderResourceContract BuildUpscalerProviderResourceContract(const UpscalerInputContract& contract) noexcept
{
	return RendererProviderResourceContract{
	    .Color = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.HudlessSceneColor)},
	    .Depth = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.Depth)},
	    .MotionVectors = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.MotionVectors)},
	    .Exposure = {.Requirement = contract.ExposureRequired ? ERendererProviderResourceRequirement::Required : ERendererProviderResourceRequirement::Optional,
	                 .Available = static_cast<bool>(contract.Exposure)},
	    .Normals = {.Requirement = ERendererProviderResourceRequirement::Optional, .Available = static_cast<bool>(contract.Normals)},
	    .History = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = true},
	    .Jitter = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = contract.RenderExtent.IsValid()},
	    .CameraMatrices = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = true},
	    .FrameIndex = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = true},
	};
}

UpscalerInputContractValidation ValidateUpscalerInputContract(const UpscalerInputContract& contract)
{
	std::vector<std::string> missing;
	const RendererProviderResourceContract resourceContract = BuildUpscalerProviderResourceContract(contract);
	AddMissing(missing, resourceContract.Color.Available, "HUD-less scene color");
	AddMissing(missing, resourceContract.Depth.Available, "depth");
	AddMissing(missing, resourceContract.MotionVectors.Available, "motion vectors");
	AddMissing(missing, static_cast<bool>(contract.FinalOutput), "final output target");
	AddMissing(missing, contract.RenderExtent.IsValid(), "valid render extent");
	AddMissing(missing, contract.OutputExtent.IsValid(), "valid output extent");
	AddMissing(missing, contract.MotionVectorConvention.Units != EUpscalerMotionVectorUnits::Unknown, "motion-vector units");
	AddMissing(missing, contract.MotionVectorConvention.Direction != EUpscalerMotionVectorDirection::Unknown, "motion-vector direction");
	AddMissing(missing, contract.DepthConvention != EUpscalerDepthConvention::Unknown, "depth convention");
	if (resourceContract.Exposure.Requirement == ERendererProviderResourceRequirement::Required)
	{
		AddMissing(missing, resourceContract.Exposure.Available, "exposure texture");
	}

	const bool valid = missing.empty();
	return UpscalerInputContractValidation{
	    .Valid = valid,
	    .MissingRequirements = std::move(missing),
	    .Summary = valid
	                   ? std::format(
	                         "valid renderExtent={}x{} outputExtent={}x{} resources={} motionVectors={} {} {} depth={} historyValid={} resetRequested={}",
	                         contract.RenderExtent.Width,
	                         contract.RenderExtent.Height,
	                         contract.OutputExtent.Width,
	                         contract.OutputExtent.Height,
	                         BuildProviderResourceContractSummary(resourceContract),
	                         UpscalerMotionVectorUnitsToString(contract.MotionVectorConvention.Units),
	                         UpscalerMotionVectorDirectionToString(contract.MotionVectorConvention.Direction),
	                         "JitterRemovedFromMotionVectors",
	                         UpscalerDepthConventionToString(contract.DepthConvention),
	                         contract.TemporalState.HistoryValid ? "true" : "false",
	                         contract.ResetRequested ? "true" : "false")
	                   : std::format(
	                         "invalid missing={} resources={}",
	                         JoinRequirements(missing),
	                         BuildProviderResourceContractSummary(resourceContract))};
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

const char* UpscalerDepthConventionToString(EUpscalerDepthConvention convention) noexcept
{
	switch (convention)
	{
		case EUpscalerDepthConvention::DeviceDepth:
			return "DeviceDepth";
		case EUpscalerDepthConvention::ReversedDeviceDepth:
			return "ReversedDeviceDepth";
		case EUpscalerDepthConvention::Unknown:
		default:
			return "Unknown";
	}
}
