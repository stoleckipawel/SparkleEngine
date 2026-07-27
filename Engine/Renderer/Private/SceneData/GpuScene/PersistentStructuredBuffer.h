#pragma once

#include "Resources/OwnedStructuredBuffer.h"
#include "SceneData/RenderSceneGpuData.h"

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
	bool Update(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);
	template <typename TValue, std::size_t Extent>
	bool Update(
	    RhiResourceService& resourceService,
	    std::span<TValue, Extent> values,
	    std::wstring_view debugName)
	{
		return Update(
		    resourceService,
		    std::as_bytes(values),
		    static_cast<std::uint32_t>(sizeof(TValue)),
		    debugName);
	}
	bool Replace(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);
	template <typename TValue, std::size_t Extent>
	bool Replace(
	    RhiResourceService& resourceService,
	    std::span<TValue, Extent> values,
	    std::wstring_view debugName)
	{
		return Replace(
		    resourceService,
		    std::as_bytes(values),
		    static_cast<std::uint32_t>(sizeof(TValue)),
		    debugName);
	}
	bool UpdateRanges(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::span<const StructuredBufferElementRange> ranges,
	    std::wstring_view debugName);
	RenderSceneGpuBuffer GetBinding() const noexcept;
	void Reset() noexcept;

  private:
	bool Grow(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);
	bool WriteDirtyRanges(std::span<const std::byte> payload);
	bool WriteRanges(
	    std::span<const std::byte> payload,
	    std::span<const StructuredBufferElementRange> ranges);
	static std::size_t ResolveCapacity(
	    std::size_t requiredSizeInBytes,
	    std::uint32_t strideInBytes) noexcept;

	OwnedStructuredBuffer m_buffer;
	std::vector<std::byte> m_shadow;
	std::size_t m_logicalSizeInBytes = 0;
	std::uint32_t m_strideInBytes = 0;
};
