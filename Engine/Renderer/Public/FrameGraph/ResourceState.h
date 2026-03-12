#pragma once

#include <cstdint>

enum class ResourceState : std::uint8_t
{
	Common,
	RenderTarget,
	DepthWrite,
	DepthRead,
	ShaderResource,
	UnorderedAccess,
	CopySource,
	CopyDest,
	Present,

	Count
};

constexpr const char* ResourceStateToString(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
			return "Common";
		case ResourceState::RenderTarget:
			return "RenderTarget";
		case ResourceState::DepthWrite:
			return "DepthWrite";
		case ResourceState::DepthRead:
			return "DepthRead";
		case ResourceState::ShaderResource:
			return "ShaderResource";
		case ResourceState::UnorderedAccess:
			return "UnorderedAccess";
		case ResourceState::CopySource:
			return "CopySource";
		case ResourceState::CopyDest:
			return "CopyDest";
		case ResourceState::Present:
			return "Present";
		default:
			return "Unknown";
	}
}
