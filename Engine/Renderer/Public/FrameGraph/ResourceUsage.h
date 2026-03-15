#pragma once

#include <cstdint>

enum class ResourceUsage : std::uint8_t
{
	RenderTarget,
	DepthRead,
	DepthWrite,
	ShaderRead,
	Present,
};

constexpr const char* ResourceUsageToString(ResourceUsage usage) noexcept
{
	switch (usage)
	{
		case ResourceUsage::RenderTarget:
			return "RenderTarget";
		case ResourceUsage::DepthRead:
			return "DepthRead";
		case ResourceUsage::DepthWrite:
			return "DepthWrite";
		case ResourceUsage::ShaderRead:
			return "ShaderRead";
		case ResourceUsage::Present:
			return "Present";
		default:
			return "Unknown";
	}
}

constexpr bool IsReadOnlyUsage(ResourceUsage usage) noexcept
{
	switch (usage)
	{
		case ResourceUsage::DepthRead:
		case ResourceUsage::ShaderRead:
		case ResourceUsage::Present:
			return true;
		default:
			return false;
	}
}

constexpr bool IsWriteOnlyUsage(ResourceUsage usage) noexcept
{
	switch (usage)
	{
		case ResourceUsage::RenderTarget:
		case ResourceUsage::DepthWrite:
			return true;
		default:
			return false;
	}
}
