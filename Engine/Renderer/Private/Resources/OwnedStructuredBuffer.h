#pragma once

#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

class RhiResourceService;

class OwnedStructuredBuffer final
{
  public:
	OwnedStructuredBuffer() noexcept = default;
	~OwnedStructuredBuffer() noexcept;

	OwnedStructuredBuffer(const OwnedStructuredBuffer&) = delete;
	OwnedStructuredBuffer& operator=(const OwnedStructuredBuffer&) = delete;
	OwnedStructuredBuffer(OwnedStructuredBuffer&& other) noexcept;
	OwnedStructuredBuffer& operator=(OwnedStructuredBuffer&& other) noexcept;

	static OwnedStructuredBuffer Upload(
	    RhiResourceService& resourceService,
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName);

	template <typename TValue>
	static OwnedStructuredBuffer Upload(
	    RhiResourceService& resourceService,
	    std::span<const TValue> values,
	    std::wstring_view debugName)
	{
		return Upload(
		    resourceService,
		    values.data(),
		    values.size_bytes(),
		    static_cast<std::uint32_t>(sizeof(TValue)),
		    debugName);
	}

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
