#include "PCH.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"

#include <cassert>
#include <string>

PassResourceBuilder::PassResourceBuilder(std::vector<PassResourceDeclaration>& declarations) noexcept : m_declarations(&declarations) {}

FrameGraphResourceHandle PassResourceBuilder::Read(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsReadOnlyUsage(usage));
	Record(handle, usage, label);
	return handle;
}

FrameGraphResourceHandle PassResourceBuilder::Write(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsWriteOnlyUsage(usage));
	Record(handle, usage, label);
	return handle;
}

FrameGraphResourceHandle PassResourceBuilder::Use(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsReadWriteUsage(usage));
	Record(handle, usage, label);
	return handle;
}

FrameGraphTextureHandle PassResourceBuilder::Read(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Read(handle.GetResourceHandle(), usage, label)};
}

FrameGraphTextureHandle PassResourceBuilder::Write(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Write(handle.GetResourceHandle(), usage, label)};
}

FrameGraphTextureHandle PassResourceBuilder::Use(FrameGraphTextureHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Use(handle.GetResourceHandle(), usage, label)};
}

FrameGraphBufferHandle PassResourceBuilder::Read(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Read(handle.GetResourceHandle(), usage, label)};
}

FrameGraphBufferHandle PassResourceBuilder::Write(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Write(handle.GetResourceHandle(), usage, label)};
}

FrameGraphBufferHandle PassResourceBuilder::Use(FrameGraphBufferHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Use(handle.GetResourceHandle(), usage, label)};
}

FrameGraphAccelerationStructureHandle PassResourceBuilder::Read(
    FrameGraphAccelerationStructureHandle handle,
    ResourceUsage usage,
    std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphAccelerationStructureHandle{Read(handle.GetResourceHandle(), usage, label)};
}

FrameGraphAccelerationStructureHandle PassResourceBuilder::Write(
    FrameGraphAccelerationStructureHandle handle,
    ResourceUsage usage,
    std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphAccelerationStructureHandle{Write(handle.GetResourceHandle(), usage, label)};
}

FrameGraphAccelerationStructureHandle PassResourceBuilder::Use(
    FrameGraphAccelerationStructureHandle handle,
    ResourceUsage usage,
    std::string_view label) noexcept
{
	assert(handle.IsValid());
	return FrameGraphAccelerationStructureHandle{Use(handle.GetResourceHandle(), usage, label)};
}

void PassResourceBuilder::Record(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(handle.IsValid());
	assert(m_declarations != nullptr);
	m_declarations->push_back(PassResourceDeclaration{.handle = handle, .usage = usage, .label = std::string(label)});
}
