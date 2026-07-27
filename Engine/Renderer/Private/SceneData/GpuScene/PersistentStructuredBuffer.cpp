#include "PCH.h"
#include "SceneData/GpuScene/PersistentStructuredBuffer.h"

#include "RHI/Public/Resources/RhiResourceService.h"

#include <algorithm>
#include <cstring>

bool PersistentStructuredBuffer::Update(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::wstring_view debugName)
{
	if (payload.empty())
	{
		Reset();
		return true;
	}
	if (strideInBytes == 0 ||
	    payload.size_bytes() % strideInBytes != 0)
	{
		return false;
	}

	if (!m_buffer || m_strideInBytes != strideInBytes ||
	    payload.size_bytes() > m_shadow.size())
	{
		return Grow(
		    resourceService,
		    payload,
		    strideInBytes,
		    debugName);
	}

	if (!WriteDirtyRanges(payload))
	{
		return false;
	}
	m_logicalSizeInBytes = payload.size_bytes();
	return true;
}

bool PersistentStructuredBuffer::Replace(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::wstring_view debugName)
{
	Reset();
	return Grow(
	    resourceService,
	    payload,
	    strideInBytes,
	    debugName);
}

bool PersistentStructuredBuffer::UpdateRanges(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::span<const StructuredBufferElementRange> ranges,
    std::wstring_view debugName)
{
	if (payload.empty())
	{
		Reset();
		return true;
	}
	if (strideInBytes == 0u ||
	    payload.size_bytes() % strideInBytes != 0u)
	{
		return false;
	}

	if (!m_buffer || m_strideInBytes != strideInBytes ||
	    payload.size_bytes() > m_shadow.size())
	{
		return Grow(resourceService, payload, strideInBytes, debugName);
	}

	if (!WriteRanges(payload, ranges))
	{
		return false;
	}

	m_logicalSizeInBytes = payload.size_bytes();
	return true;
}

RenderSceneGpuBuffer PersistentStructuredBuffer::GetBinding() const noexcept
{
	return RenderSceneGpuBuffer{
	    .Resource = m_buffer.GetResource(),
	    .SizeInBytes = m_logicalSizeInBytes,
	    .StrideInBytes = m_strideInBytes};
}

void PersistentStructuredBuffer::Reset() noexcept
{
	m_buffer.Reset();
	m_shadow.clear();
	m_logicalSizeInBytes = 0;
	m_strideInBytes = 0;
}

bool PersistentStructuredBuffer::Grow(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::wstring_view debugName)
{
	const std::size_t capacity =
	    ResolveCapacity(payload.size_bytes(), strideInBytes);
	std::vector<std::byte> shadow(capacity);
	std::copy(payload.begin(), payload.end(), shadow.begin());
	OwnedStructuredBuffer replacement = OwnedStructuredBuffer::Upload(
	    resourceService,
	    shadow.data(),
	    shadow.size(),
	    strideInBytes,
	    debugName);
	if (!replacement)
	{
		Reset();
		return false;
	}

	m_buffer = std::move(replacement);
	m_shadow = std::move(shadow);
	m_logicalSizeInBytes = payload.size_bytes();
	m_strideInBytes = strideInBytes;
	return true;
}

bool PersistentStructuredBuffer::WriteDirtyRanges(
    std::span<const std::byte> payload)
{
	const std::size_t elementCount =
	    payload.size_bytes() / m_strideInBytes;
	std::size_t elementIndex = 0;
	while (elementIndex < elementCount)
	{
		const std::size_t elementOffset =
		    elementIndex * m_strideInBytes;
		if (std::memcmp(
		        m_shadow.data() + elementOffset,
		        payload.data() + elementOffset,
		        m_strideInBytes) == 0)
		{
			++elementIndex;
			continue;
		}

		const std::size_t firstDirtyElement = elementIndex;
		do
		{
			++elementIndex;
			if (elementIndex == elementCount)
			{
				break;
			}
			const std::size_t nextOffset =
			    elementIndex * m_strideInBytes;
			if (std::memcmp(
			        m_shadow.data() + nextOffset,
			        payload.data() + nextOffset,
			        m_strideInBytes) == 0)
			{
				break;
			}
		} while (true);

		const std::size_t dirtyOffset =
		    firstDirtyElement * m_strideInBytes;
		const std::size_t dirtySize =
		    (elementIndex - firstDirtyElement) * m_strideInBytes;
		if (!m_buffer.Write(
		        dirtyOffset,
		        payload.data() + dirtyOffset,
		        dirtySize))
		{
			return false;
		}
		std::memcpy(
		    m_shadow.data() + dirtyOffset,
		    payload.data() + dirtyOffset,
		    dirtySize);
	}
	return true;
}

bool PersistentStructuredBuffer::WriteRanges(
    std::span<const std::byte> payload,
    std::span<const StructuredBufferElementRange> ranges)
{
	const std::size_t elementCount =
	    payload.size_bytes() / m_strideInBytes;
	for (const StructuredBufferElementRange& range : ranges)
	{
		if (range.ElementCount == 0u ||
		    range.FirstElement > elementCount ||
		    range.ElementCount > elementCount - range.FirstElement)
		{
			return false;
		}

		const std::size_t dirtyOffset =
		    static_cast<std::size_t>(range.FirstElement) *
		    m_strideInBytes;
		const std::size_t dirtySize =
		    static_cast<std::size_t>(range.ElementCount) *
		    m_strideInBytes;
		if (!m_buffer.Write(
		        dirtyOffset,
		        payload.data() + dirtyOffset,
		        dirtySize))
		{
			return false;
		}

		std::memcpy(
		    m_shadow.data() + dirtyOffset,
		    payload.data() + dirtyOffset,
		    dirtySize);
	}
	return true;
}

std::size_t PersistentStructuredBuffer::ResolveCapacity(
    std::size_t requiredSizeInBytes,
    std::uint32_t strideInBytes) noexcept
{
	std::size_t elementCapacity =
	    requiredSizeInBytes / strideInBytes;
	std::size_t roundedCapacity = 1;
	while (roundedCapacity < elementCapacity)
	{
		roundedCapacity *= 2;
	}
	return roundedCapacity * strideInBytes;
}
