#include "PCH.h"
#include "FrameGraph/Builder/PassResourceBuilder.h"

#include "FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h"

#include <cassert>
#include <string>
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

bool PassResourceBuilder::DeclareParameterUsages(const PassParameterSet& parameterSet, std::string_view passName) noexcept
{
	assert(m_declarations != nullptr);
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
		if (!FrameGraphResourceContractDiagnostics::ValidatePassParameterBinding(passName, parameter, *binding))
		{
			return false;
		}

		if (binding->AsDescriptorTableData() != nullptr)
		{
			continue;
		}

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

		if (parameter.ResourceDomain == ShaderParameterResourceDomain::AccelerationStructure)
		{
			DeclareAccelerationStructureBinding(parameter, *binding);
			continue;
		}

		assert(false);
	}

	return true;
}

bool PassResourceBuilder::HasFrameGraphUsage(const PassParameterDesc& parameter) noexcept
{
	switch (parameter.Kind)
	{
		case ShaderParameterSemanticKind::ReadTexture:
		case ShaderParameterSemanticKind::ReadBuffer:
		case ShaderParameterSemanticKind::RWTexture:
		case ShaderParameterSemanticKind::RWBuffer:
		case ShaderParameterSemanticKind::RenderTarget:
		case ShaderParameterSemanticKind::DepthTarget:
		case ShaderParameterSemanticKind::AccelerationStructure:
			return true;
		case ShaderParameterSemanticKind::UniformData:
		case ShaderParameterSemanticKind::SamplerSet:
			return false;
		default:
			assert(false);
			return false;
	}
}

ResourceUsage PassResourceBuilder::GetFrameGraphUsage(const PassParameterDesc& parameter) noexcept
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
		case ShaderParameterSemanticKind::AccelerationStructure:
			return ResourceUsage::AccelerationStructureRead;
		case ShaderParameterSemanticKind::UniformData:
		case ShaderParameterSemanticKind::SamplerSet:
		default:
			assert(false);
			return ResourceUsage::ShaderRead;
	}
}

void PassResourceBuilder::DeclareAccelerationStructureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept
{
	assert(parameter.ResourceDomain == ShaderParameterResourceDomain::AccelerationStructure);
	const PassParameterAccelerationStructureBindingData* accelerationStructureData = binding.AsAccelerationStructureData();
	assert(accelerationStructureData != nullptr);
	assert(accelerationStructureData->Handle.IsValid());

	DeclareResourceHandle(accelerationStructureData->Handle.GetResourceHandle(), GetFrameGraphUsage(parameter), parameter, 0);
}

void PassResourceBuilder::DeclareTextureBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept
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

void PassResourceBuilder::DeclareBufferBinding(const PassParameterDesc& parameter, const PassParameterBinding& binding) noexcept
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

void PassResourceBuilder::DeclareResourceHandle(
    FrameGraphResourceHandle handle,
    ResourceUsage usage,
    const PassParameterDesc& parameter,
    std::uint32_t arrayIndex) noexcept
{
	assert(handle.IsValid());
	assert(m_declarations != nullptr);

	std::string label = parameter.Name;
	if (parameter.ArrayCount > 1)
	{
		label += "[";
		label += std::to_string(arrayIndex);
		label += "]";
	}

	if (IsReadOnlyUsage(usage))
	{
		m_declarations->Read(handle, usage, label);
		return;
	}

	if (IsWriteOnlyUsage(usage))
	{
		m_declarations->Write(handle, usage, label);
		return;
	}

	assert(IsReadWriteUsage(usage));
	m_declarations->Use(handle, usage, label);
}
