#pragma once

#include "../RHIAPI.h"

#include "Authoring/GlobalShader.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

struct PassParameterDesc;

class SPARKLE_RHI_API ShaderPackageLayoutBuilder final
{
  public:
	ShaderPackageLayoutBuilder() = delete;

	static bool Build(
	    std::string_view packageId,
	    std::span<const ShaderRegistrationDesc> registrations,
	    PassParameterLayout& outLayout,
	    std::string& outErrorMessage);

  private:
	struct MergeEntry;

	static ShaderStageVisibility GetDefaultVisibility(ShaderStage stage) noexcept;
	static ShaderStageVisibility ResolveVisibility(
	    const ShaderParameterStructFieldDescriptor& field,
	    ShaderStage stage) noexcept;
	static PassParameterDesc BuildParameterDesc(
	    const ShaderParameterStructFieldDescriptor& field,
	    ShaderStage stage);
	static std::uint32_t GetOrderingCategory(const PassParameterDesc& parameter) noexcept;
	static bool MergeParameter(
	    std::vector<MergeEntry>& entries,
	    PassParameterDesc parameter,
	    std::uint32_t valueAlignmentInBytes,
	    const ShaderRegistrationDesc& registration,
	    std::string_view shaderStructName,
	    std::string& outErrorMessage);
	static bool AreCompatible(const MergeEntry& existing, const PassParameterDesc& incoming, std::uint32_t incomingAlignment) noexcept;
	static std::string FormatParameterDesc(const PassParameterDesc& parameter, std::uint32_t alignmentInBytes);
	static void AppendCategory(PassParameterLayout& layout, const std::vector<MergeEntry>& entries, std::uint32_t category);
};

SPARKLE_RHI_API bool BuildRegisteredShaderPackageLayout(
	std::string_view packageId,
	PassParameterLayout& outLayout,
	std::string& outErrorMessage);