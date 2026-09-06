#include "PCH.h"

#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <cassert>
#include <utility>

PassParameterValueKind PassParameterBinding::GetKind() const noexcept
{
	return std::visit(
	    [](const auto& value) noexcept -> PassParameterValueKind
	    {
		    using ValueType = std::decay_t<decltype(value)>;
		    if constexpr (std::is_same_v<ValueType, PassParameterTextureBindingData>)
		    {
			    return PassParameterValueKind::Texture;
		    }
		    else if constexpr (std::is_same_v<ValueType, PassParameterBufferBindingData>)
		    {
			    return PassParameterValueKind::Buffer;
		    }
		    else if constexpr (std::is_same_v<ValueType, PassParameterDescriptorTableBindingData>)
		    {
			    return PassParameterValueKind::DescriptorTable;
		    }
		    else if constexpr (std::is_same_v<ValueType, FrameGraphAccelerationStructureHandle>)
		    {
			    return PassParameterValueKind::AccelerationStructure;
		    }
		    else if constexpr (std::is_same_v<ValueType, PassParameterUniformBindingData>)
		    {
			    return PassParameterValueKind::UniformData;
		    }
		    else if constexpr (std::is_same_v<ValueType, PassParameterSamplerBindingData>)
		    {
			    return PassParameterValueKind::Sampler;
		    }
		    else
		    {
			    return PassParameterValueKind::None;
		    }
	    },
	    m_value);
}

bool PassParameterBinding::IsBound() const noexcept
{
	return std::visit(
	    [](const auto& value) noexcept -> bool
	    {
		    using ValueType = std::decay_t<decltype(value)>;
		    if constexpr (std::is_same_v<ValueType, std::monostate>)
		    {
			    return false;
		    }
		    else if constexpr (std::is_same_v<ValueType, FrameGraphAccelerationStructureHandle>)
		    {
			    return value.IsValid();
		    }
		    else
		    {
			    return value.IsBound();
		    }
	    },
	    m_value);
}

void PassParameterBinding::Reset() noexcept
{
	m_value.emplace<std::monostate>();
}

void PassParameterBinding::SetValue(PassParameterBindingValue value)
{
	m_value = std::move(value);
}

PassParameterSet::PassParameterSet(const PassParameterLayout& layout, std::vector<bool> graphResourceParameters) :
    m_layout(&layout),
    m_bindings(layout.GetParameterCount()),
    m_graphResourceParameters(std::move(graphResourceParameters))
{
	assert(m_graphResourceParameters.size() == layout.GetParameterCount());
}

void PassParameterSet::ClearBindings() noexcept
{
	for (PassParameterBinding& binding : m_bindings)
	{
		binding.Reset();
	}
}

const PassParameterBinding* PassParameterSet::FindBinding(const char* name) const noexcept
{
	std::uint32_t index = 0;
	return FindBinding(name, index);
}

const PassParameterBinding* PassParameterSet::GetBinding(std::uint32_t index) const noexcept
{
	return index < m_bindings.size() ? &m_bindings[index] : nullptr;
}

bool PassParameterSet::IsBound(const char* name) const noexcept
{
	const PassParameterBinding* binding = FindBinding(name);
	return binding != nullptr && binding->IsBound();
}

bool PassParameterSet::SetTexture(const char* name, FrameGraphTextureHandle handle)
{
	return SetTextureArray(name, std::vector<FrameGraphTextureHandle>{handle});
}

bool PassParameterSet::SetTextureArray(const char* name, const std::vector<FrameGraphTextureHandle>& handles)
{
	std::uint32_t index = 0;
	const PassParameterDesc* parameter = FindParameter(name, index);
	if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Texture)
	{
		return false;
	}

	if (!ValidateTextureBinding(handles, *parameter))
	{
		return false;
	}

	m_bindings[index].SetValue(PassParameterTextureBindingData{.Handles = handles});
	return true;
}

bool PassParameterSet::SetAttachment(const char* name, FrameGraphAttachmentBinding binding)
{
	std::uint32_t index = 0;
	const PassParameterDesc* parameter = FindParameter(name, index);
	if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Texture
	    || (parameter->Kind != ShaderParameterSemanticKind::RenderTarget && parameter->Kind != ShaderParameterSemanticKind::DepthTarget)
	    || !binding.Handle.IsValid())
	{
		return false;
	}

	m_bindings[index].SetValue(PassParameterTextureBindingData{.Attachment = binding});
	return true;
}

bool PassParameterSet::SetBuffer(const char* name, FrameGraphBufferHandle handle)
{
	return SetBufferArray(name, std::vector<FrameGraphBufferHandle>{handle});
}

bool PassParameterSet::SetBufferArray(const char* name, const std::vector<FrameGraphBufferHandle>& handles)
{
	std::uint32_t index = 0;
	const PassParameterDesc* parameter = FindParameter(name, index);
	if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Buffer)
	{
		return false;
	}

	if (!ValidateBufferBinding(handles, *parameter))
	{
		return false;
	}

	m_bindings[index].SetValue(PassParameterBufferBindingData{.Handles = handles});
	return true;
}

bool PassParameterSet::SetShaderResourceView(const char* name, RhiDescriptorTableBinding descriptorTable)
{
	return SetDescriptorTable(
	    name,
	    PassParameterDescriptorTableBindingData{.Table = descriptorTable},
	    ShaderParameterSemanticKind::ReadTexture,
	    ShaderParameterSemanticKind::ReadBuffer);
}

bool PassParameterSet::SetShaderResourceView(const char* name, RhiGpuDescriptorHandle descriptorTable)
{
	return SetDescriptorTable(
	    name,
	    PassParameterDescriptorTableBindingData{.GpuHandle = descriptorTable},
	    ShaderParameterSemanticKind::ReadTexture,
	    ShaderParameterSemanticKind::ReadBuffer);
}

bool PassParameterSet::UsesGraphResource(std::uint32_t index) const noexcept
{
	return index < m_graphResourceParameters.size() && m_graphResourceParameters[index];
}

bool PassParameterSet::SetUnorderedAccessView(const char* name, RhiDescriptorTableBinding descriptorTable)
{
	return SetDescriptorTable(
	    name,
	    PassParameterDescriptorTableBindingData{.Table = descriptorTable},
	    ShaderParameterSemanticKind::RWTexture,
	    ShaderParameterSemanticKind::RWBuffer);
}

bool PassParameterSet::SetUnorderedAccessView(const char* name, RhiGpuDescriptorHandle descriptorTable)
{
	return SetDescriptorTable(
	    name,
	    PassParameterDescriptorTableBindingData{.GpuHandle = descriptorTable},
	    ShaderParameterSemanticKind::RWTexture,
	    ShaderParameterSemanticKind::RWBuffer);
}

bool PassParameterSet::SetAccelerationStructure(const char* name, FrameGraphAccelerationStructureHandle handle)
{
	std::uint32_t index = 0;
	const PassParameterDesc* parameter = FindParameter(name, index);
	if (parameter == nullptr || parameter->Kind != ShaderParameterSemanticKind::AccelerationStructure)
	{
		return false;
	}

	if (parameter->ArrayCount != 1u || !handle.IsValid())
	{
		return false;
	}

	m_bindings[index].SetValue(handle);
	return true;
}

bool PassParameterSet::SetUniformDataBytes(const char* name, const void* data, std::uint32_t sizeInBytes)
{
	std::uint32_t index = 0;
	const PassParameterDesc* parameter = FindParameter(name, index);
	if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Uniform)
	{
		return false;
	}

	if (data == nullptr || sizeInBytes == 0 || parameter->ValueSizeInBytes != sizeInBytes)
	{
		return false;
	}

	m_bindings[index].SetValue(PassParameterUniformBindingData{.Data = data, .SizeInBytes = sizeInBytes});
	return true;
}

bool PassParameterSet::SetSampler(const char* name, RhiSamplerDesc sampler)
{
	std::uint32_t index = 0;
	const PassParameterDesc* parameter = FindParameter(name, index);
	if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Sampler)
	{
		return false;
	}

	m_bindings[index].SetValue(PassParameterSamplerBindingData{.Desc = sampler});
	return true;
}

bool PassParameterSet::HasAllRequiredBindings() const noexcept
{
	if (m_layout == nullptr || m_bindings.size() != m_layout->GetParameterCount())
	{
		return false;
	}

	for (const PassParameterBinding& binding : m_bindings)
	{
		if (!binding.IsBound())
		{
			return false;
		}
	}

	return true;
}

std::vector<std::string> PassParameterSet::GetMissingBindings() const
{
	std::vector<std::string> missing;
	if (m_layout == nullptr)
	{
		return missing;
	}

	const std::vector<PassParameterDesc>& parameters = m_layout->GetParameters();
	for (std::size_t index = 0; index < parameters.size() && index < m_bindings.size(); ++index)
	{
		if (!m_bindings[index].IsBound())
		{
			missing.push_back(parameters[index].Name);
		}
	}

	return missing;
}

const PassParameterDesc* PassParameterSet::FindParameter(const char* name, std::uint32_t& outIndex) const noexcept
{
	outIndex = 0;
	if (m_layout == nullptr || name == nullptr)
	{
		return nullptr;
	}

	const std::vector<PassParameterDesc>& parameters = m_layout->GetParameters();
	for (std::size_t index = 0; index < parameters.size(); ++index)
	{
		if (parameters[index].Name == name)
		{
			outIndex = static_cast<std::uint32_t>(index);
			return &parameters[index];
		}
	}

	return nullptr;
}

const PassParameterBinding* PassParameterSet::FindBinding(const char* name, std::uint32_t& outIndex) const noexcept
{
	const PassParameterDesc* parameter = FindParameter(name, outIndex);
	if (parameter == nullptr || outIndex >= m_bindings.size())
	{
		return nullptr;
	}

	return &m_bindings[outIndex];
}

bool PassParameterSet::SetDescriptorTable(
    const char* name,
    PassParameterDescriptorTableBindingData binding,
    ShaderParameterSemanticKind textureKind,
    ShaderParameterSemanticKind bufferKind)
{
	std::uint32_t index = 0;
	const PassParameterDesc* parameter = FindParameter(name, index);
	if (parameter == nullptr || (parameter->Kind != textureKind && parameter->Kind != bufferKind) || !binding.IsBound())
	{
		return false;
	}

	m_bindings[index].SetValue(binding);
	return true;
}

bool PassParameterSet::ValidateArrayCount(const PassParameterDesc& parameter, std::size_t actualCount) noexcept
{
	return parameter.ArrayCount == static_cast<std::uint32_t>(actualCount);
}

bool PassParameterSet::ValidateTextureBinding(
    const std::vector<FrameGraphTextureHandle>& handles,
    const PassParameterDesc& parameter) noexcept
{
	if (!ValidateArrayCount(parameter, handles.size()))
	{
		return false;
	}

	for (const FrameGraphTextureHandle& handle : handles)
	{
		if (!handle.IsValid())
		{
			return false;
		}
	}

	return true;
}

bool PassParameterSet::ValidateBufferBinding(
    const std::vector<FrameGraphBufferHandle>& handles,
    const PassParameterDesc& parameter) noexcept
{
	if (!ValidateArrayCount(parameter, handles.size()))
	{
		return false;
	}

	for (const FrameGraphBufferHandle& handle : handles)
	{
		if (!handle.IsValid())
		{
			return false;
		}
	}

	return true;
}
