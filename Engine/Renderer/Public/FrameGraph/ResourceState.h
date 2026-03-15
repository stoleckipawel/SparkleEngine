#pragma once

#include <cstdint>
#include <d3d12.h>

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

constexpr D3D12_RESOURCE_STATES MapToD3D12ResourceState(ResourceState state) noexcept
{
	switch (state)
	{
		case ResourceState::Common:
			return D3D12_RESOURCE_STATE_COMMON;
		case ResourceState::RenderTarget:
			return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case ResourceState::DepthWrite:
			return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case ResourceState::DepthRead:
			return D3D12_RESOURCE_STATE_DEPTH_READ;
		case ResourceState::ShaderResource:
			return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case ResourceState::UnorderedAccess:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		case ResourceState::CopySource:
			return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case ResourceState::CopyDest:
			return D3D12_RESOURCE_STATE_COPY_DEST;
		case ResourceState::Present:
			return D3D12_RESOURCE_STATE_PRESENT;
		default:
			return D3D12_RESOURCE_STATE_COMMON;
	}
}
