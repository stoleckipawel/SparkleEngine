#include "PCH.h"
#include "FrameGraph/Builder/PassBuilder.h"

#include "FrameGraph/FrameGraph.h"

#include <cassert>
#include <string>

PassBuilder::PassBuilder(FrameGraph& frameGraph) noexcept : m_frameGraph(&frameGraph) {}

FrameGraphResourceHandle PassBuilder::Read(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_frameGraph != nullptr);
	assert(IsReadOnlyUsage(usage));
	return m_frameGraph->Read(handle, usage);
}

FrameGraphResourceHandle PassBuilder::Write(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_frameGraph != nullptr);
	assert(IsWriteOnlyUsage(usage));
	return m_frameGraph->Write(handle, usage);
}

FrameGraphResourceHandle PassBuilder::Use(FrameGraphResourceHandle handle, ResourceUsage usage) noexcept
{
	assert(m_frameGraph != nullptr);
	assert(IsReadWriteUsage(usage));
	return m_frameGraph->Use(handle, usage);
}

FrameGraphTextureHandle PassBuilder::Read(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Read(handle.GetResourceHandle(), usage)};
}

FrameGraphTextureHandle PassBuilder::Write(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Write(handle.GetResourceHandle(), usage)};
}

FrameGraphTextureHandle PassBuilder::Use(FrameGraphTextureHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphTextureHandle{Use(handle.GetResourceHandle(), usage)};
}

FrameGraphBufferHandle PassBuilder::Read(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Read(handle.GetResourceHandle(), usage)};
}

FrameGraphBufferHandle PassBuilder::Write(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Write(handle.GetResourceHandle(), usage)};
}

FrameGraphBufferHandle PassBuilder::Use(FrameGraphBufferHandle handle, ResourceUsage usage) noexcept
{
	assert(handle.IsValid());
	return FrameGraphBufferHandle{Use(handle.GetResourceHandle(), usage)};
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
		case ShaderParameterSemanticKind::AccelerationStructure:
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
		case ShaderParameterSemanticKind::AccelerationStructure:
		default:
			assert(false);
			return ResourceUsage::ShaderRead;
	}
}

void PassBuilder::DeclareTextureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept
{
	assert(parameter.ResourceDomain == ShaderParameterResourceDomain::Texture);
	const PassParameterTextureBindingData* textureData = binding.AsTextureData();
	assert(textureData != nullptr);

	const ResourceUsage usage = GetFrameGraphUsage(parameter);
	for (std::uint32_t arrayIndex = 0; arrayIndex < static_cast<std::uint32_t>(textureData->Handles.size()); ++arrayIndex)
	{
		DeclareResourceHandle(textureData->Handles[arrayIndex].GetResourceHandle(), usage, parameter, arrayIndex);
	}
}

void PassBuilder::DeclareBufferBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept
{
	assert(parameter.ResourceDomain == ShaderParameterResourceDomain::Buffer);
	const PassParameterBufferBindingData* bufferData = binding.AsBufferData();
	assert(bufferData != nullptr);

	const ResourceUsage usage = GetFrameGraphUsage(parameter);
	for (std::uint32_t arrayIndex = 0; arrayIndex < static_cast<std::uint32_t>(bufferData->Handles.size()); ++arrayIndex)
	{
		DeclareResourceHandle(bufferData->Handles[arrayIndex].GetResourceHandle(), usage, parameter, arrayIndex);
	}
}

void PassBuilder::DeclareResourceHandle(
    FrameGraphResourceHandle handle,
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