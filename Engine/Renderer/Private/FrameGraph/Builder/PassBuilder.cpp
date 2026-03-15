#include "PCH.h"
#include "Renderer/Public/FrameGraph/PassBuilder.h"

#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include <cassert>

PassBuilder::PassBuilder(FrameGraph& frameGraph) noexcept : m_frameGraph(&frameGraph) {}

ResourceHandle PassBuilder::Read(ResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_frameGraph != nullptr);
	assert(IsReadOnlyUsage(usage));
	return m_frameGraph->Read(handle, usage);
}

ResourceHandle PassBuilder::Write(ResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_frameGraph != nullptr);
	assert(IsWriteOnlyUsage(usage));
	return m_frameGraph->Write(handle, usage);
}

TextureHandle PassBuilder::Read(TextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return TextureHandle{Read(handle.GetResourceHandle(), usage)};
}

TextureHandle PassBuilder::Write(TextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return TextureHandle{Write(handle.GetResourceHandle(), usage)};
}

BufferHandle PassBuilder::Read(BufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return BufferHandle{Read(handle.GetResourceHandle(), usage)};
}

BufferHandle PassBuilder::Write(BufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return BufferHandle{Write(handle.GetResourceHandle(), usage)};
}