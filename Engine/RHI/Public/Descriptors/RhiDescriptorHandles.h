#pragma once

#include "../Core/RhiGenerationalHandle.h"

#include <cstdint>

struct RhiCpuDescriptorHandle
{
	std::uintptr_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

struct RhiGpuDescriptorHandle
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

enum class ERhiDescriptorAllocatorType : std::uint8_t
{
	ShaderResource = 0,
	Sampler = 1,
	RenderTarget = 2,
	DepthStencil = 3,
};

struct RhiDescriptorAllocation
{
	RhiCpuDescriptorHandle CpuHandle = {};
	RhiGpuDescriptorHandle GpuHandle = {};

	constexpr bool IsValid() const noexcept { return static_cast<bool>(CpuHandle); }
};

struct RhiDescriptorTableHandleTag;
using RhiDescriptorTableHandle = RhiGenerationalHandle<RhiDescriptorTableHandleTag>;

struct RhiDescriptorTableBinding
{
	RhiDescriptorTableHandle Table = {};
	std::uint32_t DescriptorIndex = 0;

	constexpr explicit operator bool() const noexcept { return static_cast<bool>(Table); }
};
