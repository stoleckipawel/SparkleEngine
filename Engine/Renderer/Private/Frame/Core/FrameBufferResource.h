#pragma once

#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

class RhiResourceService;

class FrameBufferResource final
{
  public:
	FrameBufferResource() noexcept = default;
	~FrameBufferResource() noexcept;

	FrameBufferResource(const FrameBufferResource&) = delete;
	FrameBufferResource& operator=(const FrameBufferResource&) = delete;
	FrameBufferResource(FrameBufferResource&& other) noexcept;
	FrameBufferResource& operator=(FrameBufferResource&& other) noexcept;

	static FrameBufferResource Upload(
	    RhiResourceService& resourceService,
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);

	bool IsValid() const noexcept { return m_resource && m_sizeInBytes > 0 && m_strideInBytes > 0; }
	explicit operator bool() const noexcept { return IsValid(); }
	RhiOwnedResourceHandle GetResource() const noexcept { return m_resource; }
	std::uint64_t GetSizeInBytes() const noexcept { return m_sizeInBytes; }
	std::uint32_t GetStrideInBytes() const noexcept { return m_strideInBytes; }
	void Reset() noexcept;

  private:
	RhiResourceService* m_resourceService = nullptr;
	RhiOwnedResourceHandle m_resource = {};
	std::uint64_t m_sizeInBytes = 0;
	std::uint32_t m_strideInBytes = 0;
};
