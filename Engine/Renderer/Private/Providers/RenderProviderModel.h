#pragma once

#include <cstdint>
#include <string>

enum class ERendererProviderCategory : std::uint8_t
{
	Upscaler = 0,
	RayReconstruction = 1,
	FrameGeneration = 2
};

enum class ERendererProviderCapabilityState : std::uint8_t
{
	Unavailable = 0,
	MissingDependency = 1,
	UnsupportedHardware = 2,
	Available = 3,
	Enabled = 4,
	RuntimeFailed = 5
};

enum class ERendererProviderResourceRequirement : std::uint8_t
{
	Unused = 0,
	Optional = 1,
	Required = 2
};

struct RendererProviderResourceBinding final
{
	ERendererProviderResourceRequirement Requirement = ERendererProviderResourceRequirement::Unused;
	bool Available = false;
};

struct RendererProviderUpscalerResourceContract final
{
	RendererProviderResourceBinding ScalingInputColor = {};
	RendererProviderResourceBinding ScalingOutputColor = {};
	RendererProviderResourceBinding Depth = {};
	RendererProviderResourceBinding MotionVectors = {};
	RendererProviderResourceBinding Exposure = {};
	RendererProviderResourceBinding History = {};
	RendererProviderResourceBinding Jitter = {};
	RendererProviderResourceBinding CameraMatrices = {};
	RendererProviderResourceBinding FrameIndex = {};
};

bool HasMissingRequiredProviderResources(const RendererProviderUpscalerResourceContract& contract) noexcept;

std::string BuildProviderResourceContractSummary(const RendererProviderUpscalerResourceContract& contract);

const char* RendererProviderCategoryToString(ERendererProviderCategory category) noexcept;
const char* RendererProviderCapabilityStateToString(ERendererProviderCapabilityState state) noexcept;
const char* RendererProviderResourceRequirementToString(ERendererProviderResourceRequirement requirement) noexcept;
