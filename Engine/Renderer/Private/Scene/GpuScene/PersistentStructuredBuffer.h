#pragma once

#include "Resources/OwnedStructuredBuffer.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"

#include <cstdint>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

class RhiResourceService;

struct StructuredBufferElementRange final
{
	std::uint32_t FirstElement = 0u;
	std::uint32_t ElementCount = 0u;
};

class PersistentStructuredBuffer final
{
public:
	void Update(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);
	template <typename TValue, std::size_t Extent>
	void Update(RhiResourceService& resourceService, std::span<TValue, Extent> values, std::wstring_view debugName)
	{
		Update(resourceService, std::as_bytes(values), static_cast<std::uint32_t>(sizeof(TValue)), debugName);
	}
	void Replace(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);
	template <typename TValue, std::size_t Extent>
	void Replace(RhiResourceService& resourceService, std::span<TValue, Extent> values, std::wstring_view debugName)
	{
		Replace(resourceService, std::as_bytes(values), static_cast<std::uint32_t>(sizeof(TValue)), debugName);
	}
	void UpdateRanges(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::span<const StructuredBufferElementRange> ranges,
	    std::wstring_view debugName);
	RenderSceneGpuBufferBinding GetBinding() const noexcept;
	void Reset() noexcept;

private:
	void Grow(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);
	void WriteDirtyRanges(std::span<const std::byte> payload);
	void UpdateEmpty(RhiResourceService& resourceService, std::uint32_t strideInBytes, std::wstring_view debugName);
	void WriteRanges(std::span<const std::byte> payload, std::span<const StructuredBufferElementRange> ranges);
	static std::size_t ResolveCapacity(std::size_t requestedSizeInBytes, std::uint32_t strideInBytes) noexcept;

	OwnedStructuredBuffer m_buffer;
	std::vector<std::byte> m_shadow;
	std::uint32_t m_strideInBytes = 0;
};
