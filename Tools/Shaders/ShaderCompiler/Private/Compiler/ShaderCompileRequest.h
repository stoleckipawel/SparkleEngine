#pragma once

#include "RHI/Public/Shaders/ShaderTarget.h"
#include "RHI/Public/Shaders/Authoring/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/ShaderStage.h"
#include "ShaderContractCatalog.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct ShaderDescriptorBindingRemap final
{
	std::string Name;
	std::uint32_t Set = 0;
	std::uint32_t Binding = 0;
};

enum class ShaderCompileUnitKind : std::uint8_t
{
	EntryPoint = 0,
	Library,
};

enum class ShaderCompileFeatureFlags : std::uint32_t
{
	None = 0,
	InlineRayQuery = 1u << 0u,
	AccelerationStructure = 1u << 1u,
	DescriptorIndexing = 1u << 2u,
};

constexpr ShaderCompileFeatureFlags operator|(ShaderCompileFeatureFlags lhs, ShaderCompileFeatureFlags rhs) noexcept
{
	return static_cast<ShaderCompileFeatureFlags>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
}

constexpr ShaderCompileFeatureFlags& operator|=(ShaderCompileFeatureFlags& lhs, ShaderCompileFeatureFlags rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasShaderCompileFeature(ShaderCompileFeatureFlags value, ShaderCompileFeatureFlags flag) noexcept
{
	return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(flag)) != 0;
}

class ShaderSourceMountTable;

// Complete immutable input for one shader type and target compilation.
struct ShaderCompileRequest final
{
	explicit ShaderCompileRequest(const ShaderSourceMountTable& sourceMounts) noexcept :
	    SourceMounts(sourceMounts)
	{
	}

	ShaderTypeId ShaderType = 0;
	std::string ShaderTypeName;
	std::string VirtualSourcePath;
	std::reference_wrapper<const ShaderSourceMountTable> SourceMounts;
	std::string SourceCode;
	std::string EntryPoint;
	ShaderStage Stage = ShaderStage::Count;
	ShaderTarget Target = kDefaultShaderTarget;
	ShaderCompileUnitKind UnitKind = ShaderCompileUnitKind::EntryPoint;
	ShaderCompileFeatureFlags RequiredFeatures = ShaderCompileFeatureFlags::None;
	std::optional<ShaderParameterStructDescriptor> ParameterStruct;

	bool EnableDebugInfo = false;
	bool EnableOptimizations = true;
	bool TreatWarningsAsErrors = true;
	bool StripDebugInfo = true;
	bool CaptureDebugArtifacts = false;

	std::vector<std::string> Defines;
	std::vector<ShaderDescriptorBindingRemap> DescriptorBindingRemaps;
};
