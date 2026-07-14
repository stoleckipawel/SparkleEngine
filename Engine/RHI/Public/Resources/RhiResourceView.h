#pragma once

#include "../Core/RhiGenerationalHandle.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/RhiNativeHandles.h"
#include "RhiResourceDesc.h"

#include <cstdint>

struct RhiResourceViewHandleTag;
using RhiResourceViewHandle = RhiGenerationalHandle<RhiResourceViewHandleTag>;

enum class ERhiResourceViewKind : std::uint8_t
{
	TextureShaderResource = 0,
	TextureUnorderedAccess = 1,
	RenderTarget = 2,
	DepthStencil = 3,
	BufferShaderResource = 4,
	BufferUnorderedAccess = 5,
	AccelerationStructureShaderResource = 6,
};

struct RhiTextureViewRange
{
	std::uint32_t MostDetailedMip = 0;
	std::uint32_t MipCount = 1;
	std::uint32_t FirstArraySlice = 0;
	std::uint32_t ArraySize = 1;
};

struct RhiBufferViewRange
{
	std::uint64_t OffsetInBytes = 0;
	std::uint64_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;
};

struct RhiResourceViewDesc
{
	ERhiResourceViewKind Kind = ERhiResourceViewKind::TextureShaderResource;
	NativeResourceHandle Resource = {};
	PixelFormat Format = PixelFormat::Unknown;
	RhiTextureViewRange Texture = {};
	RhiBufferViewRange Buffer = {};
	RhiGpuVirtualAddress AccelerationStructureGpuAddress = 0;

	static constexpr RhiResourceViewDesc TextureShaderResource(NativeResourceHandle resource, PixelFormat format) noexcept
	{
		return RhiResourceViewDesc{.Kind = ERhiResourceViewKind::TextureShaderResource, .Resource = resource, .Format = format};
	}

	static constexpr RhiResourceViewDesc TextureUnorderedAccess(NativeResourceHandle resource, PixelFormat format) noexcept
	{
		return RhiResourceViewDesc{.Kind = ERhiResourceViewKind::TextureUnorderedAccess, .Resource = resource, .Format = format};
	}

	static constexpr RhiResourceViewDesc RenderTarget(NativeResourceHandle resource, PixelFormat format) noexcept
	{
		return RhiResourceViewDesc{.Kind = ERhiResourceViewKind::RenderTarget, .Resource = resource, .Format = format};
	}

	static constexpr RhiResourceViewDesc DepthStencil(NativeResourceHandle resource, PixelFormat format) noexcept
	{
		return RhiResourceViewDesc{.Kind = ERhiResourceViewKind::DepthStencil, .Resource = resource, .Format = format};
	}

	static constexpr RhiResourceViewDesc BufferShaderResource(
	    NativeResourceHandle resource,
	    std::uint64_t sizeInBytes,
	    std::uint32_t strideInBytes = 0,
	    std::uint64_t offsetInBytes = 0) noexcept
	{
		return RhiResourceViewDesc{
		    .Kind = ERhiResourceViewKind::BufferShaderResource,
		    .Resource = resource,
		    .Buffer = RhiBufferViewRange{.OffsetInBytes = offsetInBytes, .SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes}};
	}

	static constexpr RhiResourceViewDesc BufferUnorderedAccess(
	    NativeResourceHandle resource,
	    std::uint64_t sizeInBytes,
	    std::uint32_t strideInBytes = 0,
	    std::uint64_t offsetInBytes = 0) noexcept
	{
		return RhiResourceViewDesc{
		    .Kind = ERhiResourceViewKind::BufferUnorderedAccess,
		    .Resource = resource,
		    .Buffer = RhiBufferViewRange{.OffsetInBytes = offsetInBytes, .SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes}};
	}

	static constexpr RhiResourceViewDesc AccelerationStructureShaderResource(
	    RhiGpuVirtualAddress accelerationStructureGpuAddress) noexcept
	{
		return RhiResourceViewDesc{
		    .Kind = ERhiResourceViewKind::AccelerationStructureShaderResource,
		    .AccelerationStructureGpuAddress = accelerationStructureGpuAddress};
	}
};
