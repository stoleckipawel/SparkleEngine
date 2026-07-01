#include "PCH.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"

#include <cassert>
#include <utility>

PassResourceDeclarationSink::PassResourceDeclarationSink(std::vector<PassResourceDeclaration>& declarations) noexcept :
	m_declarations(&declarations)
{
}

FrameGraphResourceHandle PassResourceDeclarationSink::Read(
	FrameGraphResourceHandle handle,
	ResourceUsage usage,
	std::string_view label) noexcept
{
	assert(IsReadOnlyUsage(usage));
	Record(PassResourceDeclaration{.handle = handle, .usage = usage, .label = std::string(label)});
	return handle;
}

FrameGraphResourceHandle PassResourceDeclarationSink::Write(
	FrameGraphResourceHandle handle,
	ResourceUsage usage,
	std::string_view label) noexcept
{
	assert(IsWriteOnlyUsage(usage));
	Record(PassResourceDeclaration{.handle = handle, .usage = usage, .label = std::string(label)});
	return handle;
}

FrameGraphResourceHandle PassResourceDeclarationSink::Use(
	FrameGraphResourceHandle handle,
	ResourceUsage usage,
	std::string_view label) noexcept
{
	assert(IsReadWriteUsage(usage));
	Record(PassResourceDeclaration{.handle = handle, .usage = usage, .label = std::string(label)});
	return handle;
}

void PassResourceDeclarationSink::Record(PassResourceDeclaration declaration) noexcept
{
	assert(declaration.handle.IsValid());
	assert(m_declarations != nullptr);
	m_declarations->push_back(std::move(declaration));
}

PassResourceBuilder::PassResourceBuilder(PassResourceDeclarationSink& declarations) noexcept : m_declarations(&declarations) {}

FrameGraphResourceHandle PassResourceBuilder::Read(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsReadOnlyUsage(usage));
	return m_declarations->Read(handle, usage);
}

FrameGraphResourceHandle PassResourceBuilder::Write(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsWriteOnlyUsage(usage));
	return m_declarations->Write(handle, usage);
}

FrameGraphResourceHandle PassResourceBuilder::Use(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsReadWriteUsage(usage));
	return m_declarations->Use(handle, usage);
}

FrameGraphResourceHandle PassResourceBuilder::Read(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsReadOnlyUsage(usage));
	return m_declarations->Read(handle, usage, label);
}

FrameGraphResourceHandle PassResourceBuilder::Write(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsWriteOnlyUsage(usage));
	return m_declarations->Write(handle, usage, label);
}

FrameGraphResourceHandle PassResourceBuilder::Use(FrameGraphResourceHandle handle, ResourceUsage usage, std::string_view label) noexcept
{
	assert(m_declarations != nullptr);
	assert(IsReadWriteUsage(usage));
	return m_declarations->Use(handle, usage, label);
}

FrameGraphTextureHandle PassResourceBuilder::Read(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Read(handle.GetResourceHandle(), usage)};
}

FrameGraphTextureHandle PassResourceBuilder::Write(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Write(handle.GetResourceHandle(), usage)};
}

FrameGraphTextureHandle PassResourceBuilder::Use(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Use(handle.GetResourceHandle(), usage)};
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

FrameGraphBufferHandle PassResourceBuilder::Read(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Read(handle.GetResourceHandle(), usage)};
}

FrameGraphBufferHandle PassResourceBuilder::Write(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Write(handle.GetResourceHandle(), usage)};
}

FrameGraphBufferHandle PassResourceBuilder::Use(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Use(handle.GetResourceHandle(), usage)};
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

FrameGraphAccelerationStructureHandle PassResourceBuilder::Read(FrameGraphAccelerationStructureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphAccelerationStructureHandle{Read(handle.GetResourceHandle(), usage)};
}

FrameGraphAccelerationStructureHandle PassResourceBuilder::Write(FrameGraphAccelerationStructureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphAccelerationStructureHandle{Write(handle.GetResourceHandle(), usage)};
}

FrameGraphAccelerationStructureHandle PassResourceBuilder::Use(FrameGraphAccelerationStructureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphAccelerationStructureHandle{Use(handle.GetResourceHandle(), usage)};
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
