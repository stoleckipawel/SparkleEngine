#pragma once

#include "../../../RHI/Public/ShaderParameters/PassParameterLayout.h"
#include "../FrameGraph/BufferHandle.h"
#include "../FrameGraph/TextureHandle.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

struct SamplerReference
{
	std::string Name;

	bool IsValid() const noexcept { return !Name.empty(); }
};

enum class PassParameterValueKind : std::uint8_t
{
	None,
	Texture,
	Buffer,
	UniformData,
	Sampler,
};

struct PassParameterBinding
{
	PassParameterValueKind Kind = PassParameterValueKind::None;
	std::vector<TextureHandle> Textures;
	std::vector<BufferHandle> Buffers;
	const void* UniformData = nullptr;
	std::uint32_t UniformDataSizeInBytes = 0;
	SamplerReference Sampler;

	bool IsBound() const noexcept
	{
		switch (Kind)
		{
			case PassParameterValueKind::Texture:
				return !Textures.empty();
			case PassParameterValueKind::Buffer:
				return !Buffers.empty();
			case PassParameterValueKind::UniformData:
				return UniformData != nullptr && UniformDataSizeInBytes > 0;
			case PassParameterValueKind::Sampler:
				return Sampler.IsValid();
			default:
				return false;
		}
	}

	void Reset() noexcept
	{
		Kind = PassParameterValueKind::None;
		Textures.clear();
		Buffers.clear();
		UniformData = nullptr;
		UniformDataSizeInBytes = 0;
		Sampler = {};
	}
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

		PassParameterBinding& binding = m_bindings[index];
		binding.Reset();
		binding.Kind = PassParameterValueKind::Texture;
		binding.Textures = handles;
		return ValidateTextureBinding(binding.Textures, *parameter);
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

		PassParameterBinding& binding = m_bindings[index];
		binding.Reset();
		binding.Kind = PassParameterValueKind::Buffer;
		binding.Buffers = handles;
		return ValidateBufferBinding(binding.Buffers, *parameter);
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

		PassParameterBinding& binding = m_bindings[index];
		binding.Reset();
		binding.Kind = PassParameterValueKind::UniformData;
		binding.UniformData = data;
		binding.UniformDataSizeInBytes = sizeInBytes;
		return true;
	}

	bool SetSampler(const char* name, SamplerReference sampler)
	{
		std::uint32_t index = 0;
		const PassParameterDesc* parameter = FindParameter(name, index);
		if (parameter == nullptr || parameter->ResourceDomain != ShaderParameterResourceDomain::Sampler)
		{
			return false;
		}

		if (!sampler.IsValid())
		{
			return false;
		}

		PassParameterBinding& binding = m_bindings[index];
		binding.Reset();
		binding.Kind = PassParameterValueKind::Sampler;
		binding.Sampler = std::move(sampler);
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