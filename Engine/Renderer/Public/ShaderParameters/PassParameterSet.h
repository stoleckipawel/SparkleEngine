#pragma once

#include "../../../RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "../../../RHI/Public/Interop/RenderHardwareInterface.h"
#include "../FrameGraph/BufferHandle.h"
#include "../FrameGraph/TextureHandle.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

enum class PassParameterValueKind : std::uint8_t
{
	None,
	Texture,
	Buffer,
	DescriptorTable,
	AccelerationStructure,
	UniformData,
	Sampler,
};

struct PassParameterTextureBindingData
{
	std::vector<TextureHandle> Handles;

	bool IsBound() const noexcept { return !Handles.empty(); }
};

struct PassParameterBufferBindingData
{
	std::vector<BufferHandle> Handles;

	bool IsBound() const noexcept { return !Handles.empty(); }
};

struct PassParameterDescriptorTableBindingData
{
	RhiDescriptorTableBinding Table = {};

	bool IsBound() const noexcept { return static_cast<bool>(Table); }
};

struct PassParameterAccelerationStructureBindingData
{
	RhiGpuVirtualAddress GpuAddress = 0;

	bool IsBound() const noexcept { return true; }
};

struct PassParameterUniformBindingData
{
	const void* Data = nullptr;
	std::uint32_t SizeInBytes = 0;

	bool IsBound() const noexcept { return Data != nullptr && SizeInBytes > 0; }
};

struct PassParameterSamplerBindingData
{
	RhiSamplerDesc Desc = {};

	bool IsBound() const noexcept { return true; }
};

using PassParameterBindingValue = std::variant<
	std::monostate,
	PassParameterTextureBindingData,
	PassParameterBufferBindingData,
	PassParameterDescriptorTableBindingData,
	PassParameterAccelerationStructureBindingData,
	PassParameterUniformBindingData,
	PassParameterSamplerBindingData>;

struct PassParameterBinding
{
	PassParameterValueKind GetKind() const noexcept
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
			    else if constexpr (std::is_same_v<ValueType, PassParameterAccelerationStructureBindingData>)
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

	bool IsBound() const noexcept
	{
		return std::visit(
		    [](const auto& value) noexcept -> bool
		    {
			    using ValueType = std::decay_t<decltype(value)>;
			    if constexpr (std::is_same_v<ValueType, std::monostate>)
			    {
				    return false;
			    }
			    else
			    {
				    return value.IsBound();
			    }
		    },
		    m_value);
	}

	const PassParameterTextureBindingData* AsTextureData() const noexcept
	{
		return std::get_if<PassParameterTextureBindingData>(&m_value);
	}

	const PassParameterBufferBindingData* AsBufferData() const noexcept
	{
		return std::get_if<PassParameterBufferBindingData>(&m_value);
	}

	const PassParameterDescriptorTableBindingData* AsDescriptorTableData() const noexcept
	{
		return std::get_if<PassParameterDescriptorTableBindingData>(&m_value);
	}

	const PassParameterAccelerationStructureBindingData* AsAccelerationStructureData() const noexcept
	{
		return std::get_if<PassParameterAccelerationStructureBindingData>(&m_value);
	}

	const PassParameterUniformBindingData* AsUniformData() const noexcept
	{
		return std::get_if<PassParameterUniformBindingData>(&m_value);
	}

	const PassParameterSamplerBindingData* AsSamplerData() const noexcept
	{
		return std::get_if<PassParameterSamplerBindingData>(&m_value);
	}

  private:
	friend class PassParameterSet;

	void Reset() noexcept { m_value.emplace<std::monostate>(); }

	void SetValue(PassParameterBindingValue value) { m_value = std::move(value); }

	PassParameterBindingValue m_value;
};

class PassParameterSet final
{
  public:
	PassParameterSet() = default;
	explicit PassParameterSet(const PassParameterLayout& layout) { Reset(layout); }

	void Reset(const PassParameterLayout& layout)
	{
		m_layout = &layout;
		m_bindings.clear();
		m_bindings.resize(layout.GetParameterCount());
	}

	void ClearBindings() noexcept
	{
		for (PassParameterBinding& binding : m_bindings)
		{
			binding.Reset();
		}
	}

	const PassParameterLayout* GetLayout() const noexcept { return m_layout; }

	bool HasLayout() const noexcept { return m_layout != nullptr; }

	std::size_t GetBindingCount() const noexcept { return m_bindings.size(); }

	const PassParameterBinding* FindBinding(const char* name) const noexcept
	{
		std::uint32_t index = 0;
		return FindBinding(name, index);
	}

	const PassParameterBinding* GetBinding(std::uint32_t index) const noexcept
	{
		return index < m_bindings.size() ? &m_bindings[index] : nullptr;
	}

	bool IsBound(const char* name) const noexcept
	{
		const PassParameterBinding* binding = FindBinding(name);
		return binding != nullptr && binding->IsBound();
	}

	bool SetTexture(const char* name, TextureHandle handle)
	{
		std::vector<TextureHandle> handles;
		handles.push_back(handle);
		return SetTextureArray(name, handles);
	}

	bool SetTextureArray(const char* name, const std::vector<TextureHandle>& handles)
	{
		std::uint32_t index = 0;
		const PassParameterDesc* parameter = FindParameter(name, index);
		if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Texture)
		{
			return false;
		}

		if (!ValidateArrayCount(*parameter, handles.size()))
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

	bool SetBuffer(const char* name, BufferHandle handle)
	{
		std::vector<BufferHandle> handles;
		handles.push_back(handle);
		return SetBufferArray(name, handles);
	}

	bool SetBufferArray(const char* name, const std::vector<BufferHandle>& handles)
	{
		std::uint32_t index = 0;
		const PassParameterDesc* parameter = FindParameter(name, index);
		if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Buffer)
		{
			return false;
		}

		if (!ValidateArrayCount(*parameter, handles.size()))
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

	bool SetShaderResourceView(const char* name, RhiDescriptorTableBinding descriptorTable)
	{
		std::uint32_t index = 0;
		const PassParameterDesc* parameter = FindParameter(name, index);
		if (parameter == nullptr ||
		    (parameter->Kind != ShaderParameterSemanticKind::ReadTexture && parameter->Kind != ShaderParameterSemanticKind::ReadBuffer))
		{
			return false;
		}

		if (!descriptorTable || parameter->ArrayCount != 1u)
		{
			return false;
		}

		m_bindings[index].SetValue(PassParameterDescriptorTableBindingData{.Table = descriptorTable});
		return true;
	}

	bool SetUnorderedAccessView(const char* name, RhiDescriptorTableBinding descriptorTable)
	{
		std::uint32_t index = 0;
		const PassParameterDesc* parameter = FindParameter(name, index);
		if (parameter == nullptr ||
		    (parameter->Kind != ShaderParameterSemanticKind::RWTexture && parameter->Kind != ShaderParameterSemanticKind::RWBuffer))
		{
			return false;
		}

		if (!descriptorTable || parameter->ArrayCount != 1u)
		{
			return false;
		}

		m_bindings[index].SetValue(PassParameterDescriptorTableBindingData{.Table = descriptorTable});
		return true;
	}

	bool SetAccelerationStructure(const char* name, RhiGpuVirtualAddress gpuAddress)
	{
		std::uint32_t index = 0;
		const PassParameterDesc* parameter = FindParameter(name, index);
		if (parameter == nullptr || parameter->Kind != ShaderParameterSemanticKind::AccelerationStructure)
		{
			return false;
		}

		if (parameter->ArrayCount != 1u)
		{
			return false;
		}

		m_bindings[index].SetValue(PassParameterAccelerationStructureBindingData{.GpuAddress = gpuAddress});
		return true;
	}

	template <typename T> bool SetUniformDataReference(const char* name, const T& value)
	{
		static_assert(std::is_trivially_copyable_v<T>, "Uniform data must be trivially copyable.");
		static_assert(std::is_standard_layout_v<T>, "Uniform data must be standard layout.");

		return SetUniformDataBytes(name, &value, static_cast<std::uint32_t>(sizeof(T)));
	}

	bool SetUniformDataBytes(const char* name, const void* data, std::uint32_t sizeInBytes)
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

	bool SetSampler(const char* name, RhiSamplerDesc sampler)
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

	bool HasAllRequiredBindings() const noexcept
	{
		if (m_layout == nullptr || m_bindings.size() != m_layout->GetParameterCount())
		{
			return false;
		}

		for (std::size_t i = 0; i < m_bindings.size(); ++i)
		{
			if (!m_bindings[i].IsBound())
			{
				return false;
			}
		}

		return true;
	}

	std::vector<std::string> GetMissingBindings() const
	{
		std::vector<std::string> missing;
		if (m_layout == nullptr)
		{
			return missing;
		}

		const std::vector<PassParameterDesc>& parameters = m_layout->GetParameters();
		for (std::size_t i = 0; i < parameters.size() && i < m_bindings.size(); ++i)
		{
			if (!m_bindings[i].IsBound())
			{
				missing.push_back(parameters[i].Name);
			}
		}

		return missing;
	}

  private:
	const PassParameterDesc* FindParameter(const char* name, std::uint32_t& outIndex) const noexcept
	{
		outIndex = 0;
		if (m_layout == nullptr || name == nullptr)
		{
			return nullptr;
		}

		const std::vector<PassParameterDesc>& parameters = m_layout->GetParameters();
		for (std::size_t i = 0; i < parameters.size(); ++i)
		{
			if (parameters[i].Name == name)
			{
				outIndex = static_cast<std::uint32_t>(i);
				return &parameters[i];
			}
		}

		return nullptr;
	}

	const PassParameterBinding* FindBinding(const char* name, std::uint32_t& outIndex) const noexcept
	{
		const PassParameterDesc* parameter = FindParameter(name, outIndex);
		if (parameter == nullptr || outIndex >= m_bindings.size())
		{
			return nullptr;
		}

		return &m_bindings[outIndex];
	}

	static bool ValidateArrayCount(const PassParameterDesc& parameter, std::size_t actualCount) noexcept
	{
		return parameter.ArrayCount == static_cast<std::uint32_t>(actualCount);
	}

	static bool ValidateTextureBinding(const std::vector<TextureHandle>& handles, const PassParameterDesc& parameter) noexcept
	{
		if (!ValidateArrayCount(parameter, handles.size()))
		{
			return false;
		}

		for (const TextureHandle& handle : handles)
		{
			if (!handle.IsValid())
			{
				return false;
			}
		}

		return true;
	}

	static bool ValidateBufferBinding(const std::vector<BufferHandle>& handles, const PassParameterDesc& parameter) noexcept
	{
		if (!ValidateArrayCount(parameter, handles.size()))
		{
			return false;
		}

		for (const BufferHandle& handle : handles)
		{
			if (!handle.IsValid())
			{
				return false;
			}
		}

		return true;
	}

	const PassParameterLayout* m_layout = nullptr;
	std::vector<PassParameterBinding> m_bindings;
};