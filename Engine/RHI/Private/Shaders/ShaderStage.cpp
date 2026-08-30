#include "PCH.h"

#include "Shaders/ShaderStage.h"

#include <array>
#include <cstddef>

namespace ShaderStageFormatting
{
	struct ShaderStageLabel final
	{
		ShaderStageMask Mask;
		const char* Label;
	};

	constexpr std::array<const char*, 12> Prefixes = {"vs", "ps", "gs", "hs", "ds", "cs", "rgs", "miss", "chs", "ahs", "is", "callable"};
	constexpr std::array<ShaderStageLabel, 12> Labels = {{
	    {ShaderStageMask::Vertex, "Vertex"},
	    {ShaderStageMask::Pixel, "Pixel"},
	    {ShaderStageMask::Geometry, "Geometry"},
	    {ShaderStageMask::Hull, "Hull"},
	    {ShaderStageMask::Domain, "Domain"},
	    {ShaderStageMask::Compute, "Compute"},
	    {ShaderStageMask::RayGeneration, "RayGeneration"},
	    {ShaderStageMask::Miss, "Miss"},
	    {ShaderStageMask::ClosestHit, "ClosestHit"},
	    {ShaderStageMask::AnyHit, "AnyHit"},
	    {ShaderStageMask::Intersection, "Intersection"},
	    {ShaderStageMask::Callable, "Callable"},
	}};
}

ShaderStageMask operator|(ShaderStageMask lhs, ShaderStageMask rhs) noexcept
{
	return static_cast<ShaderStageMask>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

ShaderStageMask operator&(ShaderStageMask lhs, ShaderStageMask rhs) noexcept
{
	return static_cast<ShaderStageMask>(static_cast<std::uint16_t>(lhs) & static_cast<std::uint16_t>(rhs));
}

ShaderStageMask& operator|=(ShaderStageMask& lhs, ShaderStageMask rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

bool HasAnyShaderStageMask(ShaderStageMask value, ShaderStageMask flags) noexcept
{
	return static_cast<std::uint16_t>(value & flags) != 0;
}

ShaderStageMask ToShaderStageMask(ShaderStage stage) noexcept
{
	switch (stage)
	{
		case ShaderStage::Vertex:
			return ShaderStageMask::Vertex;
		case ShaderStage::Pixel:
			return ShaderStageMask::Pixel;
		case ShaderStage::Geometry:
			return ShaderStageMask::Geometry;
		case ShaderStage::Hull:
			return ShaderStageMask::Hull;
		case ShaderStage::Domain:
			return ShaderStageMask::Domain;
		case ShaderStage::Compute:
			return ShaderStageMask::Compute;
		case ShaderStage::RayGeneration:
			return ShaderStageMask::RayGeneration;
		case ShaderStage::Miss:
			return ShaderStageMask::Miss;
		case ShaderStage::ClosestHit:
			return ShaderStageMask::ClosestHit;
		case ShaderStage::AnyHit:
			return ShaderStageMask::AnyHit;
		case ShaderStage::Intersection:
			return ShaderStageMask::Intersection;
		case ShaderStage::Callable:
			return ShaderStageMask::Callable;
		case ShaderStage::Count:
		default:
			return ShaderStageMask::None;
	}
}

bool IsRayTracingShaderStage(ShaderStage stage) noexcept
{
	return stage >= ShaderStage::RayGeneration && stage <= ShaderStage::Callable;
}

const char* GetShaderStagePrefix(ShaderStage stage) noexcept
{
	if (stage == ShaderStage::Count)
	{
		return "lib";
	}
	return ShaderStageFormatting::Prefixes[static_cast<std::size_t>(stage)];
}

std::string FormatShaderStageMask(ShaderStageMask mask)
{
	if (mask == ShaderStageMask::None)
	{
		return "None";
	}

	std::string result;
	for (const ShaderStageFormatting::ShaderStageLabel& stageLabel : ShaderStageFormatting::Labels)
	{
		if (!HasAnyShaderStageMask(mask, stageLabel.Mask))
		{
			continue;
		}
		if (!result.empty())
		{
			result += '|';
		}
		result += stageLabel.Label;
	}
	return result.empty() ? "None" : result;
}
