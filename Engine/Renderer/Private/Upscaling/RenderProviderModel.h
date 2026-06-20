#pragma once

#include <cstdint>
#include <string>

enum class ERendererProviderCategory : std::uint8_t
{
	Upscaler = 0,
	Denoiser = 1,
	FrameGeneration = 2,
	RayTracingExtension = 3,
	NeuralRendering = 4
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

struct RendererProviderResourceContract final
{
	RendererProviderResourceBinding Color = {};
	RendererProviderResourceBinding Depth = {};
	RendererProviderResourceBinding MotionVectors = {};
	RendererProviderResourceBinding Exposure = {};
	RendererProviderResourceBinding Normals = {};
	RendererProviderResourceBinding History = {};
	RendererProviderResourceBinding Jitter = {};
	RendererProviderResourceBinding CameraMatrices = {};
	RendererProviderResourceBinding FrameIndex = {};
};

bool HasMissingRequiredProviderResources(const RendererProviderResourceContract& contract) noexcept;

std::string BuildProviderResourceContractSummary(const RendererProviderResourceContract& contract);

const char* RendererProviderCategoryToString(ERendererProviderCategory category) noexcept;
const char* RendererProviderCapabilityStateToString(ERendererProviderCapabilityState state) noexcept;
const char* RendererProviderResourceRequirementToString(ERendererProviderResourceRequirement requirement) noexcept;
