#pragma once

#include <cstdint>
#include <string>
#include <string_view>

struct FrameGraphBufferDesc
{
	std::string name;
	std::uint64_t sizeInBytes = 0;
	std::uint32_t strideInBytes = 0;

	static FrameGraphBufferDesc Create(std::string_view name, std::uint64_t sizeInBytes, std::uint32_t strideInBytes = 0) noexcept
	{
		return FrameGraphBufferDesc{std::string(name), sizeInBytes, strideInBytes};
	}
};