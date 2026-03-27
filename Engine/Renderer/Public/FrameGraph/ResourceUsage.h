#pragma once

#include <cstdint>

enum class ResourceUsage : std::uint8_t
{
	RenderTarget,
	DepthRead,
	DepthWrite,
	ShaderRead,
	UnorderedAccess,
	CopySource,
	CopyDest,
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
		case ResourceUsage::UnorderedAccess:
			return "UnorderedAccess";
		case ResourceUsage::CopySource:
			return "CopySource";
		case ResourceUsage::CopyDest:
			return "CopyDest";
		case ResourceUsage::Present:
			return "Present";
		default:
			return "Unknown";
	}
}

constexpr bool IsReadWriteUsage(ResourceUsage usage) noexcept
{
	switch (usage)
	{
		case ResourceUsage::UnorderedAccess:
			return true;
		default:
			return false;
	}
}

constexpr bool IsReadOnlyUsage(ResourceUsage usage) noexcept
{
	switch (usage)
	{
		case ResourceUsage::DepthRead:
		case ResourceUsage::ShaderRead:
		case ResourceUsage::CopySource:
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
		case ResourceUsage::CopyDest:
			return true;
		default:
			return false;
	}
}

constexpr bool ReadsFromUsage(ResourceUsage usage) noexcept
{
	return IsReadOnlyUsage(usage) || IsReadWriteUsage(usage);
}

constexpr bool WritesToUsage(ResourceUsage usage) noexcept
{
	return IsWriteOnlyUsage(usage) || IsReadWriteUsage(usage);
}

constexpr bool UsesUnorderedAccess(ResourceUsage usage) noexcept
{
	return usage == ResourceUsage::UnorderedAccess;
}
