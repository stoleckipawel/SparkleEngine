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
}

bool HasMissingRequiredProviderResources(const RendererProviderUpscalerResourceContract& contract) noexcept
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

std::string BuildProviderResourceContractSummary(const RendererProviderUpscalerResourceContract& contract)
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

const char* RendererProviderCategoryToString(ERendererProviderCategory category) noexcept
{
	switch (category)
	{
		case ERendererProviderCategory::Upscaler:
			return "upscaler";
		case ERendererProviderCategory::Denoiser:
			return "denoiser";
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
