#include "PCH.h"
#include "Renderer/Public/FrameGraph/PassBuilder.h"

#include "FrameGraph/FrameGraph.h"

#include <cassert>
#include <string>

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

ResourceHandle PassBuilder::Use(ResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_frameGraph != nullptr);
	assert(IsReadWriteUsage(usage));
	return m_frameGraph->Use(handle, usage);
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

TextureHandle PassBuilder::Use(TextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return TextureHandle{Use(handle.GetResourceHandle(), usage)};
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

BufferHandle PassBuilder::Use(BufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return BufferHandle{Use(handle.GetResourceHandle(), usage)};
}

void PassBuilder::DeclareParameterUsages(const PassParameterSet& parameterSet) noexcept
{
	assert(m_frameGraph != nullptr);
	assert(parameterSet.HasLayout());

	const PassParameterLayout* layout = parameterSet.GetLayout();
	assert(layout != nullptr);

	const std::vector<PassParameterDesc>& parameters = layout->GetParameters();
	assert(parameterSet.GetBindingCount() == parameters.size());

	for (std::uint32_t index = 0; index < static_cast<std::uint32_t>(parameters.size()); ++index)
	{
		const PassParameterDesc& parameter = parameters[index];
		if (!HasFrameGraphUsage(parameter))
		{
			continue;
		}

		const PassParameterBinding* binding = parameterSet.GetBinding(index);
		assert(binding != nullptr);
		assert(binding->IsBound());

		if (parameter.ResourceDomain == ShaderParameterResourceDomain::Texture)
		{
			DeclareTextureBinding(parameter, *binding);
			continue;
		}

		if (parameter.ResourceDomain == ShaderParameterResourceDomain::Buffer)
		{
			DeclareBufferBinding(parameter, *binding);
			continue;
		}

		assert(false);
	}
}

bool PassBuilder::HasFrameGraphUsage(const PassParameterDesc& parameter) noexcept
{
	switch (parameter.Kind)
	{
		case ShaderParameterSemanticKind::ReadTexture:
		case ShaderParameterSemanticKind::ReadBuffer:
		case ShaderParameterSemanticKind::RWTexture:
		case ShaderParameterSemanticKind::RWBuffer:
		case ShaderParameterSemanticKind::RenderTarget:
		case ShaderParameterSemanticKind::DepthTarget:
			return true;
		case ShaderParameterSemanticKind::UniformData:
		case ShaderParameterSemanticKind::SamplerSet:
			return false;
		default:
			assert(false);
			return false;
	}
}

ResourceUsage PassBuilder::GetFrameGraphUsage(const PassParameterDesc& parameter) noexcept
{
	switch (parameter.Kind)
	{
		case ShaderParameterSemanticKind::ReadTexture:
		case ShaderParameterSemanticKind::ReadBuffer:
			return ResourceUsage::ShaderRead;
		case ShaderParameterSemanticKind::RWTexture:
		case ShaderParameterSemanticKind::RWBuffer:
			return ResourceUsage::UnorderedAccess;
		case ShaderParameterSemanticKind::RenderTarget:
			return ResourceUsage::RenderTarget;
		case ShaderParameterSemanticKind::DepthTarget:
			return ResourceUsage::DepthWrite;
		case ShaderParameterSemanticKind::UniformData:
		case ShaderParameterSemanticKind::SamplerSet:
		default:
			assert(false);
			return ResourceUsage::ShaderRead;
	}
}

void PassBuilder::DeclareTextureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept
{
	assert(parameter.ResourceDomain == ShaderParameterResourceDomain::Texture);
	assert(binding.Kind == PassParameterValueKind::Texture);

	const ResourceUsage usage = GetFrameGraphUsage(parameter);
	for (std::uint32_t arrayIndex = 0; arrayIndex < static_cast<std::uint32_t>(binding.Textures.size()); ++arrayIndex)
	{
		DeclareResourceHandle(binding.Textures[arrayIndex].GetResourceHandle(), usage, parameter, arrayIndex);
	}
}

void PassBuilder::DeclareBufferBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept
{
	assert(parameter.ResourceDomain == ShaderParameterResourceDomain::Buffer);
	assert(binding.Kind == PassParameterValueKind::Buffer);

	const ResourceUsage usage = GetFrameGraphUsage(parameter);
	for (std::uint32_t arrayIndex = 0; arrayIndex < static_cast<std::uint32_t>(binding.Buffers.size()); ++arrayIndex)
	{
		DeclareResourceHandle(binding.Buffers[arrayIndex].GetResourceHandle(), usage, parameter, arrayIndex);
	}
}

void PassBuilder::DeclareResourceHandle(
    ResourceHandle handle,
    ResourceUsage usage,
    const PassParameterDesc& parameter,
    std::uint32_t arrayIndex) noexcept
{
	assert(handle.IsValid());
	assert(m_frameGraph != nullptr);

	std::string label = parameter.Name;
	if (parameter.ArrayCount > 1)
	{
		label += "[";
		label += std::to_string(arrayIndex);
		label += "]";
	}

	if (IsReadOnlyUsage(usage))
	{
		m_frameGraph->Read(handle, usage, label);
		return;
	}

	if (IsWriteOnlyUsage(usage))
	{
		m_frameGraph->Write(handle, usage, label);
		return;
	}

	assert(IsReadWriteUsage(usage));
	m_frameGraph->Use(handle, usage, label);
}