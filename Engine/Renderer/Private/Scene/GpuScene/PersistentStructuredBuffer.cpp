#include "PCH.h"
#include "Scene/GpuScene/PersistentStructuredBuffer.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Resources/RhiResourceService.h"

#include <algorithm>
#include <cstring>

static const auto g_persistentStructuredBufferLogger = Logging::GetOrCreateLogger("Renderer.PersistentStructuredBuffer");

void PersistentStructuredBuffer::Update(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::wstring_view debugName)
{
	if (strideInBytes == 0 || payload.size_bytes() % strideInBytes != 0)
	{
		Diagnostics::Fatal(
		    g_persistentStructuredBufferLogger,
		    __FILE__,
		    __LINE__,
		    "Persistent structured-buffer update has an invalid stride or payload size.");
	}
	if (payload.empty())
	{
		UpdateEmpty(resourceService, strideInBytes, debugName);
		return;
	}

	if (!m_buffer || m_strideInBytes != strideInBytes || payload.size_bytes() > m_shadow.size())
	{
		Grow(resourceService, payload, strideInBytes, debugName);
		return;
	}

	WriteDirtyRanges(payload);
}

void PersistentStructuredBuffer::Replace(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::wstring_view debugName)
{
	Reset();
	Grow(resourceService, payload, strideInBytes, debugName);
}

void PersistentStructuredBuffer::UpdateRanges(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::span<const StructuredBufferElementRange> ranges,
    std::wstring_view debugName)
{
	if (strideInBytes == 0u || payload.size_bytes() % strideInBytes != 0u)
	{
		Diagnostics::Fatal(
		    g_persistentStructuredBufferLogger,
		    __FILE__,
		    __LINE__,
		    "Persistent structured-buffer range update has an invalid stride or payload size.");
	}
	if (payload.empty())
	{
		UpdateEmpty(resourceService, strideInBytes, debugName);
		return;
	}

	if (!m_buffer || m_strideInBytes != strideInBytes || payload.size_bytes() > m_shadow.size())
	{
		Grow(resourceService, payload, strideInBytes, debugName);
		return;
	}

	WriteRanges(payload, ranges);
}

RenderSceneGpuBufferBinding PersistentStructuredBuffer::GetBinding() const noexcept
{
	return RenderSceneGpuBufferBinding{
	    .Resource = m_buffer.GetResource(),
	    .SizeInBytes = m_shadow.size(),
	    .StrideInBytes = m_strideInBytes};
}

void PersistentStructuredBuffer::Reset() noexcept
{
	m_buffer.Reset();
	m_shadow.clear();
	m_strideInBytes = 0;
}

void PersistentStructuredBuffer::Grow(
    RhiResourceService& resourceService,
    std::span<const std::byte> payload,
    std::uint32_t strideInBytes,
    std::wstring_view debugName)
{
	if (strideInBytes == 0u || payload.size_bytes() % strideInBytes != 0u)
	{
		Diagnostics::Fatal(
		    g_persistentStructuredBufferLogger,
		    __FILE__,
		    __LINE__,
		    "Persistent structured-buffer allocation has an invalid stride or payload size.");
	}
	const std::size_t capacity = ResolveCapacity(payload.size_bytes(), strideInBytes);
	std::vector<std::byte> shadow(capacity);
	std::copy(payload.begin(), payload.end(), shadow.begin());
	OwnedStructuredBuffer replacement =
	    OwnedStructuredBuffer::Upload(resourceService, shadow.data(), shadow.size(), strideInBytes, debugName);
	if (!replacement)
	{
		Diagnostics::Fatal(
		    g_persistentStructuredBufferLogger,
		    __FILE__,
		    __LINE__,
		    "Persistent structured-buffer allocation or upload failed.");
	}

	// ReleaseOwnedResource retires the replaced allocation against its recorded GPU submissions; growth never requires device idle.
	m_buffer = std::move(replacement);
	m_shadow = std::move(shadow);
	m_strideInBytes = strideInBytes;
}

void PersistentStructuredBuffer::UpdateEmpty(RhiResourceService& resourceService, std::uint32_t strideInBytes, std::wstring_view debugName)
{
	if (!m_buffer || m_strideInBytes != strideInBytes)
	{
		Grow(resourceService, {}, strideInBytes, debugName);
		return;
	}

	const auto firstNonZero =
	    std::find_if(m_shadow.begin(), m_shadow.begin() + strideInBytes, [](std::byte value) { return value != std::byte{}; });
	if (firstNonZero == m_shadow.begin() + strideInBytes)
	{
		return;
	}

	std::vector<std::byte> zeroElement(strideInBytes);
	if (!m_buffer.Write(0u, zeroElement.data(), zeroElement.size()))
	{
		Diagnostics::Fatal(
		    g_persistentStructuredBufferLogger,
		    __FILE__,
		    __LINE__,
		    "Persistent structured-buffer zero-element update failed.");
	}
	std::copy(zeroElement.begin(), zeroElement.end(), m_shadow.begin());
}

void PersistentStructuredBuffer::WriteDirtyRanges(std::span<const std::byte> payload)
{
	const std::size_t elementCount = payload.size_bytes() / m_strideInBytes;
	std::size_t elementIndex = 0;
	while (elementIndex < elementCount)
	{
		const std::size_t elementOffset = elementIndex * m_strideInBytes;
		if (std::memcmp(m_shadow.data() + elementOffset, payload.data() + elementOffset, m_strideInBytes) == 0)
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
			const std::size_t nextOffset = elementIndex * m_strideInBytes;
			if (std::memcmp(m_shadow.data() + nextOffset, payload.data() + nextOffset, m_strideInBytes) == 0)
			{
				break;
			}
		} while (true);

		const std::size_t dirtyOffset = firstDirtyElement * m_strideInBytes;
		const std::size_t dirtySize = (elementIndex - firstDirtyElement) * m_strideInBytes;
		if (!m_buffer.Write(dirtyOffset, payload.data() + dirtyOffset, dirtySize))
		{
			Diagnostics::Fatal(
			    g_persistentStructuredBufferLogger,
			    __FILE__,
			    __LINE__,
			    "Persistent structured-buffer dirty-range update failed.");
		}
		std::memcpy(m_shadow.data() + dirtyOffset, payload.data() + dirtyOffset, dirtySize);
	}
}

void PersistentStructuredBuffer::WriteRanges(std::span<const std::byte> payload, std::span<const StructuredBufferElementRange> ranges)
{
	const std::size_t elementCount = payload.size_bytes() / m_strideInBytes;
	for (const StructuredBufferElementRange& range : ranges)
	{
		if (range.ElementCount == 0u || range.FirstElement > elementCount || range.ElementCount > elementCount - range.FirstElement)
		{
			Diagnostics::Fatal(
			    g_persistentStructuredBufferLogger,
			    __FILE__,
			    __LINE__,
			    "Persistent structured-buffer update range exceeds the payload.");
		}

		const std::size_t dirtyOffset = static_cast<std::size_t>(range.FirstElement) * m_strideInBytes;
		const std::size_t dirtySize = static_cast<std::size_t>(range.ElementCount) * m_strideInBytes;
		if (!m_buffer.Write(dirtyOffset, payload.data() + dirtyOffset, dirtySize))
		{
			Diagnostics::Fatal(g_persistentStructuredBufferLogger, __FILE__, __LINE__, "Persistent structured-buffer range update failed.");
		}

		std::memcpy(m_shadow.data() + dirtyOffset, payload.data() + dirtyOffset, dirtySize);
	}
}

std::size_t PersistentStructuredBuffer::ResolveCapacity(std::size_t requestedSizeInBytes, std::uint32_t strideInBytes) noexcept
{
	std::size_t elementCapacity = requestedSizeInBytes / strideInBytes;
	std::size_t roundedCapacity = 1;
	while (roundedCapacity < elementCapacity)
	{
		roundedCapacity *= 2;
	}
	return roundedCapacity * strideInBytes;
}
