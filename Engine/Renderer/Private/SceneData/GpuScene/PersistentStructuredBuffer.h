#pragma once

#include "Resources/OwnedStructuredBuffer.h"
#include "SceneData/RenderSceneGpuData.h"

#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

class RhiResourceService;

class PersistentStructuredBuffer final
{
  public:
	bool Update(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);
	bool Replace(
	    RhiResourceService& resourceService,
	    std::span<const std::byte> payload,
	    std::uint32_t strideInBytes,
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
	static std::size_t ResolveCapacity(
	    std::size_t requiredSizeInBytes,
	    std::uint32_t strideInBytes) noexcept;

	OwnedStructuredBuffer m_buffer;
	std::vector<std::byte> m_shadow;
	std::size_t m_logicalSizeInBytes = 0;
	std::uint32_t m_strideInBytes = 0;
};
