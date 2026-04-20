#include "PCH.h"

#include "Manifest/ShaderStageNames.h"

#include "Core/Public/Strings/StringUtils.h"

const std::array<ShaderStageNames::Entry, ShaderStageNames::kEntryCount> ShaderStageNames::kTable = {
	Entry{ShaderStage::Vertex, ShaderStageMask::Vertex, std::string_view{"Vertex"}},
	Entry{ShaderStage::Pixel, ShaderStageMask::Pixel, std::string_view{"Pixel"}},
	Entry{ShaderStage::Geometry, ShaderStageMask::Geometry, std::string_view{"Geometry"}},
	Entry{ShaderStage::Hull, ShaderStageMask::Hull, std::string_view{"Hull"}},
	Entry{ShaderStage::Domain, ShaderStageMask::Domain, std::string_view{"Domain"}},
	Entry{ShaderStage::Compute, ShaderStageMask::Compute, std::string_view{"Compute"}}};

std::optional<ShaderStage> ShaderStageNames::TryParse(std::string_view name) noexcept
{
	for (const Entry& entry : kTable)
	{
		if (Engine::Strings::EqualsIgnoreCase(name, entry.name))
		{
			return entry.stage;
		}
	}
	return std::nullopt;
}

std::string_view ShaderStageNames::ToString(ShaderStage stage) noexcept
{
	for (const Entry& entry : kTable)
	{
		if (entry.stage == stage)
		{
			return entry.name;
		}
	}
	return std::string_view{};
}

std::string ShaderStageNames::FormatMask(ShaderStageMask mask)
{
	std::string value;
	for (const Entry& entry : kTable)
	{
		if (!HasAnyShaderStageMask(mask, entry.mask))
		{
			continue;
		}

		if (!value.empty())
		{
			value += '|';
		}
		value.append(entry.name);
	}

	return value.empty() ? std::string{"None"} : value;
}
