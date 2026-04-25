#include "PCH.h"

#include "Cooking/ShaderStageText.h"

#include <array>

std::string ShaderStageText::FormatMask(ShaderStageMask mask)
{
	struct StageEntry final
	{
		ShaderStageMask mask;
		const char* name;
	};

	static constexpr std::array<StageEntry, 6> kStages = {{
	    {ShaderStageMask::Vertex, "Vertex"},
	    {ShaderStageMask::Pixel, "Pixel"},
	    {ShaderStageMask::Geometry, "Geometry"},
	    {ShaderStageMask::Hull, "Hull"},
	    {ShaderStageMask::Domain, "Domain"},
	    {ShaderStageMask::Compute, "Compute"},
	}};

	std::string value;
	for (const StageEntry& stage : kStages)
	{
		if (!HasAnyShaderStageMask(mask, stage.mask))
		{
			continue;
		}

		if (!value.empty())
		{
			value += '|';
		}
		value += stage.name;
	}

	return value.empty() ? std::string{"None"} : value;
}