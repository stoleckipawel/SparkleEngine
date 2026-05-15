#pragma once

#include "../Formats/PixelFormat.h"
#include "../Interop/ResourceState.h"

#include <array>
#include <cstdint>

using RhiGpuVirtualAddress = std::uint64_t;

enum class RhiPrimitiveTopology : std::uint8_t
{
	TriangleList = 0,
};

enum class RhiIndexFormat : std::uint8_t
{
	UInt16 = 0,
	UInt32 = 1,
};

struct RhiVertexBufferView
{
	RhiGpuVirtualAddress BufferLocation = 0;
	std::uint32_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;
};

struct RhiIndexBufferView
{
	RhiGpuVirtualAddress BufferLocation = 0;
	std::uint32_t SizeInBytes = 0;
	RhiIndexFormat Format = RhiIndexFormat::UInt32;
};

struct RhiViewport
{
	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
	float MinDepth = 0.0f;
	float MaxDepth = 1.0f;
};

struct RhiRect
{
	std::int32_t Left = 0;
	std::int32_t Top = 0;
	std::int32_t Right = 0;
	std::int32_t Bottom = 0;
};

enum class RhiTransientAllocationPool : std::uint8_t
{
	Color = 0,
	Depth = 1,
	Buffer = 2,
};

struct RhiTextureResourceDesc
{
	std::uint32_t Width = 1;
	std::uint32_t Height = 1;
	PixelFormat Format = PixelFormat::Unknown;
	std::uint16_t MipLevels = 1;
	bool AllowRenderTarget = false;
	bool AllowDepthStencil = false;
	bool AllowUnorderedAccess = false;
};

struct RhiBufferResourceDesc
{
	std::uint64_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;
	bool AllowUnorderedAccess = false;
};

struct RhiResourceAllocationInfo
{
	std::uint64_t SizeInBytes = 0;
	std::uint64_t Alignment = 0;
};

struct RhiOptimizedClearValue
{
	enum class Type : std::uint8_t
	{
		None = 0,
		Color = 1,
		DepthStencil = 2,
	};

	Type ValueType = Type::None;
	PixelFormat Format = PixelFormat::Unknown;
	std::array<float, 4> Color = {0.0f, 0.0f, 0.0f, 1.0f};
	float Depth = 1.0f;
	std::uint8_t Stencil = 0;
};

struct RhiTransientTextureAllocationDesc
{
	RhiTextureResourceDesc ResourceDesc = {};
	RhiOptimizedClearValue ClearValue = {};
	ResourceState InitialState = ResourceState::Common;
};

struct RhiTransientBufferAllocationDesc
{
	RhiBufferResourceDesc ResourceDesc = {};
	ResourceState InitialState = ResourceState::Common;
};
