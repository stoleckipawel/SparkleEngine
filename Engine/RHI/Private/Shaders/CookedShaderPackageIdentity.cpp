#include "PCH.h"

#include "Shaders/CookedShaderPackageIdentity.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "ShaderParameters/PassParameterLayout.h"
#include "Shaders/CookedShaderPackageContract.h"

#include <format>
#include <string>

class CookedShaderPackageCanonicalization final
{
public:
	static std::string NormalizeShaderPackageToken(std::string_view value, std::string_view fallback = {})
	{
		std::string normalized = Strings::ToLowerCopy(Strings::TrimAsciiWhitespace(value));
		if (!normalized.empty())
		{
			return normalized;
		}

		return std::string(fallback);
	}

	static std::uint32_t ToLayoutVisibilityBits(ShaderStageVisibility visibility) noexcept
	{
		switch (visibility)
		{
			case ShaderStageVisibility::Vertex:
				return static_cast<std::uint32_t>(ShaderStageMask::Vertex);
			case ShaderStageVisibility::Pixel:
				return static_cast<std::uint32_t>(ShaderStageMask::Pixel);
			case ShaderStageVisibility::Compute:
				return static_cast<std::uint32_t>(ShaderStageMask::Compute);
			case ShaderStageVisibility::AllGraphics:
				return static_cast<std::uint32_t>(ShaderStageMask::Vertex | ShaderStageMask::Pixel);
			case ShaderStageVisibility::All:
				return static_cast<std::uint32_t>(ShaderStageMask::Vertex | ShaderStageMask::Pixel | ShaderStageMask::Compute);
			case ShaderStageVisibility::None:
			default:
				return static_cast<std::uint32_t>(ShaderStageMask::None);
		}
	}
};

bool ShaderPackageDefinition::IsValid() const noexcept
{
	return PackageId != nullptr && PackageId[0] != '\0' && ExpectedStages != ShaderStageMask::None;
}

std::uint64_t BuildShaderPackageKey(std::string_view packageId)
{
	return Hash::Fnv1a64(CookedShaderPackageCanonicalization::NormalizeShaderPackageToken(packageId));
}

std::uint64_t BuildShaderBlobId(
    std::string_view packageId,
    std::string_view entryPoint,
    std::string_view exportName,
    std::string_view compilerBackendName,
    std::string_view codegenTarget,
    CookedShaderBinaryFormat binaryFormat)
{
	std::string canonical;
	canonical.reserve(192);
	canonical += "ShaderBlobId.v1|PackageId=";
	canonical += CookedShaderPackageCanonicalization::NormalizeShaderPackageToken(packageId);
	canonical += "|EntryPoint=";
	canonical += Strings::TrimAsciiWhitespace(entryPoint);
	canonical += "|ExportName=";
	canonical += Strings::TrimAsciiWhitespace(exportName);
	canonical += "|CompilerBackend=";
	canonical += CookedShaderPackageCanonicalization::NormalizeShaderPackageToken(compilerBackendName);
	canonical += "|CodegenTarget=";
	canonical += Strings::TrimAsciiWhitespace(codegenTarget);
	canonical += "|BinaryFormat=";
	canonical += CookedShaderBinaryFormatToString(binaryFormat);

	const std::uint64_t hash = Hash::Fnv1a64(canonical);
	return hash != 0 ? hash : Hash::kFnv64OffsetBasis;
}

std::uint64_t BuildPassParameterLayoutHash(const PassParameterLayout& layout)
{
	std::string canonicalLayout;
	canonicalLayout.reserve(layout.GetParameterCount() * 64u);
	canonicalLayout += std::format("count={};", layout.GetParameterCount());

	for (const PassParameterDesc& parameter : layout.GetParameters())
	{
		canonicalLayout += parameter.Name;
		canonicalLayout += '|';
		canonicalLayout += std::to_string(static_cast<std::uint32_t>(parameter.Kind));
		canonicalLayout += '|';
		canonicalLayout += std::to_string(static_cast<std::uint32_t>(parameter.ResourceDomain));
		canonicalLayout += '|';
		canonicalLayout += std::to_string(static_cast<std::uint32_t>(parameter.Access));
		canonicalLayout += '|';
		canonicalLayout += std::to_string(CookedShaderPackageCanonicalization::ToLayoutVisibilityBits(parameter.Visibility));
		canonicalLayout += '|';
		canonicalLayout += std::to_string(parameter.ArrayCount);
		canonicalLayout += '|';
		canonicalLayout += std::to_string(parameter.ValueSizeInBytes);
		canonicalLayout += ';';
	}

	return Hash::Fnv1a64(canonicalLayout);
}

const char* CookedShaderBinaryFormatToString(CookedShaderBinaryFormat format) noexcept
{
	switch (format)
	{
		case CookedShaderBinaryFormat::Dxil:
			return "DXIL";
		case CookedShaderBinaryFormat::SpirV:
			return "SPIR-V";
	}

	return "unknown";
}

std::string_view GetRuntimeShaderCodegenTarget(CookedShaderBinaryFormat format) noexcept
{
	switch (format)
	{
		case CookedShaderBinaryFormat::Dxil:
			return CookedShaderPackageContract::DxilRuntimeCodegenTarget;
		case CookedShaderBinaryFormat::SpirV:
			return CookedShaderPackageContract::SpirVRuntimeCodegenTarget;
	}

	return {};
}
