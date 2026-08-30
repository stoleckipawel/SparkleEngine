#include "../PCH.h"
#include "Resources/OwnedStructuredBuffer.h"

#include "RHI/Public/Resources/RhiResourceService.h"

#include <utility>

OwnedStructuredBuffer::~OwnedStructuredBuffer() noexcept
{
	Reset();
}

OwnedStructuredBuffer::OwnedStructuredBuffer(OwnedStructuredBuffer&& other) noexcept
{
	*this = std::move(other);
}

OwnedStructuredBuffer& OwnedStructuredBuffer::operator=(OwnedStructuredBuffer&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	Reset();
	m_resourceService = other.m_resourceService;
	m_resource = other.m_resource;
	m_sizeInBytes = other.m_sizeInBytes;
	m_strideInBytes = other.m_strideInBytes;
	other.m_resourceService = nullptr;
	other.m_resource = {};
	other.m_sizeInBytes = 0;
	other.m_strideInBytes = 0;
	return *this;
}

OwnedStructuredBuffer OwnedStructuredBuffer::Upload(
    RhiResourceService& resourceService,
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName)
{
	OwnedStructuredBuffer buffer;
	buffer.m_resourceService = &resourceService;
	if (!resourceService.CreateStructuredBufferResource(data, sizeInBytes, strideInBytes, debugName, buffer.m_resource))
	{
		return {};
	}

	buffer.m_sizeInBytes = sizeInBytes;
	buffer.m_strideInBytes = strideInBytes;
	return buffer;
}

bool OwnedStructuredBuffer::Write(std::size_t destinationOffsetInBytes, const void* data, std::size_t sizeInBytes) noexcept
{
	return m_resourceService != nullptr && m_resource && destinationOffsetInBytes <= m_sizeInBytes
	    && sizeInBytes <= m_sizeInBytes - destinationOffsetInBytes
	    && m_resourceService->WriteBufferResource(m_resource, destinationOffsetInBytes, data, sizeInBytes);
}

void OwnedStructuredBuffer::Reset() noexcept
{
	if (m_resourceService != nullptr && m_resource)
	{
		m_resourceService->ReleaseOwnedResource(m_resource);
	}
	m_resourceService = nullptr;
	m_resource = {};
	m_sizeInBytes = 0;
	m_strideInBytes = 0;
}
