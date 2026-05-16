#include "PCH.h"

#include "Shaders/CookedShaderPackageUtils.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "ShaderParameters/PassParameterLayout.h"

#include <format>
#include <string>

namespace
{
	std::string NormalizeShaderPackageToken(std::string_view value, std::string_view fallback = {})
	{
		std::string normalized = Strings::ToLowerCopy(Strings::TrimAsciiWhitespace(value));
		if (!normalized.empty())
		{
			return normalized;
		}

		return fallback.empty() ? std::string{} : std::string(fallback);
	}

	std::uint32_t ToLayoutVisibilityBits(ShaderStageVisibility visibility) noexcept
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
}  // namespace

std::uint64_t BuildShaderPackageKey(std::string_view packageId)
{
	const std::string normalizedPackageId = NormalizeShaderPackageToken(packageId);
	return Hash::Fnv1a64(normalizedPackageId);
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
		canonicalLayout += parameter.GetShaderName();
		canonicalLayout += '|';
		canonicalLayout += std::to_string(static_cast<std::uint32_t>(parameter.Kind));
		canonicalLayout += '|';
		canonicalLayout += std::to_string(static_cast<std::uint32_t>(parameter.ResourceDomain));
		canonicalLayout += '|';
		canonicalLayout += std::to_string(static_cast<std::uint32_t>(parameter.Access));
		canonicalLayout += '|';
		canonicalLayout += std::to_string(ToLayoutVisibilityBits(parameter.Visibility));
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
