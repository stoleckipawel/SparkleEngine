#include "PCH.h"

#include "Cooking/StageMaskUtils.h"

ShaderStageMask StageMaskUtils::FromVisibility(ShaderStageVisibility visibility) noexcept
{
	switch (visibility)
	{
		case ShaderStageVisibility::Vertex:
			return ShaderStageMask::Vertex;
		case ShaderStageVisibility::Pixel:
			return ShaderStageMask::Pixel;
		case ShaderStageVisibility::Compute:
			return ShaderStageMask::Compute;
		case ShaderStageVisibility::AllGraphics:
			return ShaderStageMask::Vertex | ShaderStageMask::Pixel;
		case ShaderStageVisibility::All:
			return ShaderStageMask::Vertex | ShaderStageMask::Pixel | ShaderStageMask::Compute;
		case ShaderStageVisibility::None:
		default:
			return ShaderStageMask::None;
	}
}

		}
