#include "../PCH.h"
#include "Providers/RenderProviderModel.h"

#include <array>
#include <format>
#include <string_view>

namespace
{
	struct NamedProviderResourceBinding final
	{
		std::string_view Name;
		const RendererProviderResourceBinding* Binding = nullptr;
	};

	constexpr std::array<NamedProviderResourceBinding, 9> GetNamedBindings(const RendererProviderUpscalerResourceContract& contract) noexcept
	{
		return {{
		    {"scaling input color", &contract.ScalingInputColor},
		    {"scaling output color", &contract.ScalingOutputColor},
		    {"depth", &contract.Depth},
		    {"motion vectors", &contract.MotionVectors},
		    {"exposure", &contract.Exposure},
		    {"history", &contract.History},
		    {"jitter", &contract.Jitter},
		    {"camera matrices", &contract.CameraMatrices},
		    {"frame index", &contract.FrameIndex},
		}};
	}

	constexpr std::array<NamedProviderResourceBinding, 14> GetNamedBindings(
	    const RendererProviderRayReconstructionResourceContract& contract) noexcept
	{
		return {{
		    {"noisy input color", &contract.NoisyInputColor},
		    {"output color", &contract.OutputColor},
		    {"depth", &contract.Depth},
		    {"motion vectors", &contract.MotionVectors},
		    {"normals", &contract.Normals},
		    {"roughness", &contract.Roughness},
		    {"diffuse albedo", &contract.DiffuseAlbedo},
		    {"specular albedo", &contract.SpecularAlbedo},
		    {"specular hit distance", &contract.SpecularHitDistance},
		    {"exposure", &contract.Exposure},
		    {"history", &contract.History},
		    {"jitter", &contract.Jitter},
		    {"camera matrices", &contract.CameraMatrices},
		    {"frame index", &contract.FrameIndex},
		}};
	}

	template <typename TContract>
	bool HasMissingRequiredResources(const TContract& contract) noexcept
	{
		for (const NamedProviderResourceBinding& namedBinding : GetNamedBindings(contract))
		{
			if (namedBinding.Binding->Requirement == ERendererProviderResourceRequirement::Required && !namedBinding.Binding->Available)
			{
				return true;
			}
		}

		return false;
	}

	template <typename TContract>
	std::string BuildResourceContractSummary(const TContract& contract)
	{
		std::string summary;
		for (const NamedProviderResourceBinding& namedBinding : GetNamedBindings(contract))
		{
			if (namedBinding.Binding->Requirement == ERendererProviderResourceRequirement::Unused)
			{
				continue;
			}

			if (!summary.empty())
			{
				summary += ", ";
			}

			summary += std::format(
			    "{}={}{}",
			    namedBinding.Name,
			    RendererProviderResourceRequirementToString(namedBinding.Binding->Requirement),
			    namedBinding.Binding->Available ? "" : ":missing");
		}

		return summary;
	}
}

bool HasMissingRequiredProviderResources(const RendererProviderUpscalerResourceContract& contract) noexcept
{
	return HasMissingRequiredResources(contract);
}

bool HasMissingRequiredProviderResources(const RendererProviderRayReconstructionResourceContract& contract) noexcept
{
	return HasMissingRequiredResources(contract);
}

std::string BuildProviderResourceContractSummary(const RendererProviderUpscalerResourceContract& contract)
{
	return BuildResourceContractSummary(contract);
}

std::string BuildProviderResourceContractSummary(const RendererProviderRayReconstructionResourceContract& contract)
{
	return BuildResourceContractSummary(contract);
}

const char* RendererProviderCategoryToString(ERendererProviderCategory category) noexcept
{
	switch (category)
	{
		case ERendererProviderCategory::Upscaler:
			return "upscaler";
		case ERendererProviderCategory::RayReconstruction:
			return "ray reconstruction";
		case ERendererProviderCategory::FrameGeneration:
			return "frame generation";
	}

	return "unknown";
}

const char* RendererProviderCapabilityStateToString(ERendererProviderCapabilityState state) noexcept
{
	switch (state)
	{
		case ERendererProviderCapabilityState::Unavailable:
			return "unavailable";
		case ERendererProviderCapabilityState::MissingDependency:
			return "missing dependency";
		case ERendererProviderCapabilityState::UnsupportedHardware:
			return "unsupported hardware";
		case ERendererProviderCapabilityState::Available:
			return "available";
		case ERendererProviderCapabilityState::Enabled:
			return "enabled";
		case ERendererProviderCapabilityState::RuntimeFailed:
			return "runtime failed";
	}

	return "unknown";
}

const char* RendererProviderResourceRequirementToString(ERendererProviderResourceRequirement requirement) noexcept
{
	switch (requirement)
	{
		case ERendererProviderResourceRequirement::Unused:
			return "unused";
		case ERendererProviderResourceRequirement::Optional:
			return "optional";
		case ERendererProviderResourceRequirement::Required:
			return "required";
	}

	return "unknown";
}
