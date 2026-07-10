#include "../PCH.h"
#include "RayReconstruction/RayReconstructionInputContract.h"

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

	void AddMissingRequired(
	    std::vector<std::string>& missing,
	    const RendererProviderResourceBinding& binding,
	    std::string_view label)
	{
		if (binding.Requirement == ERendererProviderResourceRequirement::Required && !binding.Available)
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

RendererProviderRayReconstructionResourceContract BuildRayReconstructionProviderResourceContract(
    const RayReconstructionInputContract& contract) noexcept
{
	return RendererProviderRayReconstructionResourceContract{
	    .NoisyInputColor = {.Requirement = ERendererProviderResourceRequirement::Required,
	                        .Available = static_cast<bool>(contract.NoisyInputColor)},
	    .OutputColor = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.OutputColor)},
	    .Depth = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.Depth)},
	    .MotionVectors = {.Requirement = ERendererProviderResourceRequirement::Required,
	                      .Available = static_cast<bool>(contract.MotionVectors)},
	    .Normals = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.Normals)},
	    .Roughness = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.Roughness)},
	    .DiffuseAlbedo = {.Requirement = ERendererProviderResourceRequirement::Required,
	                      .Available = static_cast<bool>(contract.DiffuseAlbedo)},
	    .SpecularAlbedo = {.Requirement = ERendererProviderResourceRequirement::Required,
	                       .Available = static_cast<bool>(contract.SpecularAlbedo)},
	    .SpecularHitDistance = {.Requirement = ERendererProviderResourceRequirement::Required,
	                            .Available = static_cast<bool>(contract.SpecularHitDistance)},
	    .Exposure = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = static_cast<bool>(contract.Exposure)},
	    .History = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = true},
	    .Jitter = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = contract.RenderExtent.IsValid()},
	    .CameraMatrices = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = true},
	    .FrameIndex = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = true},
	};
}

RayReconstructionInputContractValidation ValidateRayReconstructionInputContract(const RayReconstructionInputContract& contract)
{
	std::vector<std::string> missing;
	const RendererProviderRayReconstructionResourceContract resourceContract =
	    BuildRayReconstructionProviderResourceContract(contract);
	AddMissingRequired(missing, resourceContract.NoisyInputColor, "noisy input color");
	AddMissingRequired(missing, resourceContract.OutputColor, "output color");
	AddMissingRequired(missing, resourceContract.Depth, "depth");
	AddMissingRequired(missing, resourceContract.MotionVectors, "motion vectors");
	AddMissingRequired(missing, resourceContract.Normals, "normals");
	AddMissingRequired(missing, resourceContract.Roughness, "roughness");
	AddMissingRequired(missing, resourceContract.DiffuseAlbedo, "diffuse albedo");
	AddMissingRequired(missing, resourceContract.SpecularAlbedo, "specular albedo");
	AddMissingRequired(missing, resourceContract.SpecularHitDistance, "specular hit distance");
	AddMissingRequired(missing, resourceContract.Exposure, "exposure");
	AddMissing(missing, contract.RenderExtent.IsValid(), "valid render extent");
	AddMissing(missing, contract.OutputExtent.IsValid(), "valid output extent");
	AddMissing(missing, contract.MotionVectorConvention.Units != ERayReconstructionMotionVectorUnits::Unknown, "motion-vector units");
	AddMissing(
	    missing,
	    contract.MotionVectorConvention.Direction != ERayReconstructionMotionVectorDirection::Unknown,
	    "motion-vector direction");
	AddMissing(missing, contract.DepthConvention != ERayReconstructionDepthConvention::Unknown, "depth convention");

	const bool valid = missing.empty();
	return RayReconstructionInputContractValidation{
	    .Valid = valid,
	    .MissingRequirements = std::move(missing),
	    .Summary = valid
	                   ? std::format(
	                         "valid extent={}x{} resources={} motionVectors={} {} depth={} historyValid={} resetRequested={}",
	                         contract.RenderExtent.Width,
	                         contract.RenderExtent.Height,
	                         BuildProviderResourceContractSummary(resourceContract),
	                         RayReconstructionMotionVectorUnitsToString(contract.MotionVectorConvention.Units),
	                         RayReconstructionMotionVectorDirectionToString(contract.MotionVectorConvention.Direction),
	                         RayReconstructionDepthConventionToString(contract.DepthConvention),
	                         contract.TemporalState.HistoryValid ? "true" : "false",
	                         contract.ResetRequested ? "true" : "false")
	                   : std::format(
	                         "invalid missing={} resources={}",
	                         JoinRequirements(missing),
	                         BuildProviderResourceContractSummary(resourceContract))};
}

const char* RayReconstructionMotionVectorUnitsToString(ERayReconstructionMotionVectorUnits units) noexcept
{
	switch (units)
	{
		case ERayReconstructionMotionVectorUnits::PixelDelta:
			return "PixelDelta";
		case ERayReconstructionMotionVectorUnits::Unknown:
		default:
			return "Unknown";
	}
}

const char* RayReconstructionMotionVectorDirectionToString(ERayReconstructionMotionVectorDirection direction) noexcept
{
	switch (direction)
	{
		case ERayReconstructionMotionVectorDirection::CurrentMinusPrevious:
			return "CurrentMinusPrevious";
		case ERayReconstructionMotionVectorDirection::Unknown:
		default:
			return "Unknown";
	}
}

const char* RayReconstructionDepthConventionToString(ERayReconstructionDepthConvention convention) noexcept
{
	switch (convention)
	{
		case ERayReconstructionDepthConvention::DeviceDepth:
			return "DeviceDepth";
		case ERayReconstructionDepthConvention::ReversedDeviceDepth:
			return "ReversedDeviceDepth";
		case ERayReconstructionDepthConvention::LinearDepth:
			return "LinearDepth";
		case ERayReconstructionDepthConvention::Unknown:
		default:
			return "Unknown";
	}
}
