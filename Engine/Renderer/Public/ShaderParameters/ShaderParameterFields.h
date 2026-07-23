#pragma once

#include "PassParameterSet.h"
#include "../FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/RendererAPI.h"

#include <array>
#include <cstddef>
#include <type_traits>
#include <vector>

namespace ShaderParameterFields
{
	template <typename THandle, std::size_t ArrayCount> class ResourceArrayField
	{
	  public:
		static_assert(ArrayCount > 0, "Shader parameter field arrays must contain at least one element.");

		using HandleType = THandle;
		static constexpr std::size_t Count = ArrayCount;

		ResourceArrayField() = default;

		template <std::size_t CountValue = ArrayCount, typename = std::enable_if_t<CountValue == 1>>
		ResourceArrayField& operator=(THandle handle) noexcept
		{
			m_values[0] = handle;
			return *this;
		}

		void Set(std::size_t index, THandle handle) noexcept { m_values[index] = handle; }

		const THandle& operator[](std::size_t index) const noexcept { return m_values[index]; }

		THandle& operator[](std::size_t index) noexcept { return m_values[index]; }

		const std::array<THandle, ArrayCount>& GetValues() const noexcept { return m_values; }

		std::vector<THandle> ToVector() const { return std::vector<THandle>(m_values.begin(), m_values.end()); }

		bool IsBound() const noexcept
		{
			for (const THandle& value : m_values)
			{
				if (!value.IsValid())
				{
					return false;
				}
			}

			return true;
		}

	  private:
		std::array<THandle, ArrayCount> m_values = {};
	};
}  // namespace ShaderParameterFields

template <typename TValue = void, std::size_t ArrayCount = 1>
class ShaderTexture2D final : public ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>;
	using Semantic = ReadTexture;
	using ValueType = TValue;
	using Base::operator=;
	using Base::Base;
};

template <typename TValue = void, std::size_t ArrayCount = 1>
class ShaderTexture3D final : public ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>;
	using Semantic = ReadTexture;
	using ValueType = TValue;
	using Base::operator=;
	using Base::Base;
};

template <typename TValue = void, std::size_t ArrayCount = 1>
class ShaderTextureCube final : public ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>;
	using Semantic = ReadTexture;
	using ValueType = TValue;
	using Base::operator=;
	using Base::Base;
};

class SPARKLE_RENDERER_API ShaderTexture2DSRV final
{
  public:
	using Semantic = ReadTexture;

	ShaderTexture2DSRV();

	ShaderTexture2DSRV& operator=(RhiDescriptorTableBinding descriptorTable) noexcept
	{
		m_descriptorTable = descriptorTable;
		return *this;
	}

	RhiDescriptorTableBinding GetDescriptorTable() const noexcept { return m_descriptorTable; }

	bool IsBound() const noexcept { return static_cast<bool>(m_descriptorTable); }

	void Reset() noexcept { m_descriptorTable = {}; }

  private:
	RhiDescriptorTableBinding m_descriptorTable = {};
};

template <std::size_t ArrayCount> class ShaderTexture2DTableSRV final
{
  public:
	static_assert(ArrayCount > 0, "Shader texture descriptor tables must contain at least one element.");

	using Semantic = ReadTexture;
	static constexpr std::size_t Count = ArrayCount;

	ShaderTexture2DTableSRV() = default;

	ShaderTexture2DTableSRV& operator=(RhiDescriptorTableBinding descriptorTable) noexcept
	{
		m_descriptorTable = descriptorTable;
		return *this;
	}

	RhiDescriptorTableBinding GetDescriptorTable() const noexcept { return m_descriptorTable; }

	bool IsBound() const noexcept { return static_cast<bool>(m_descriptorTable); }

	void Reset() noexcept { m_descriptorTable = {}; }

  private:
	RhiDescriptorTableBinding m_descriptorTable = {};
};

class SPARKLE_RENDERER_API ShaderAccelerationStructure final
{
  public:
	using Semantic = AccelerationStructure;

	ShaderAccelerationStructure();

	ShaderAccelerationStructure& operator=(RhiGpuVirtualAddress gpuAddress) noexcept
	{
		m_handle = FrameGraphAccelerationStructureHandle::Invalid();
		m_gpuAddress = gpuAddress;
		m_isBound = true;
		return *this;
	}

	ShaderAccelerationStructure& operator=(FrameGraphAccelerationStructureHandle handle) noexcept
	{
		m_handle = handle;
		m_gpuAddress = 0;
		m_isBound = handle.IsValid();
		return *this;
	}

	RhiGpuVirtualAddress GetGpuAddress() const noexcept { return m_gpuAddress; }
	FrameGraphAccelerationStructureHandle GetHandle() const noexcept { return m_handle; }

	bool IsBound() const noexcept { return m_isBound; }

	void Reset() noexcept
	{
		m_handle = FrameGraphAccelerationStructureHandle::Invalid();
		m_gpuAddress = 0;
		m_isBound = false;
	}

  private:
	FrameGraphAccelerationStructureHandle m_handle = FrameGraphAccelerationStructureHandle::Invalid();
	RhiGpuVirtualAddress m_gpuAddress = 0;
	bool m_isBound = false;
};

template <typename TValue = void, std::size_t ArrayCount = 1>
class ShaderRWTexture2D final : public ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, ArrayCount>;
	using Semantic = RWTexture;
	using ValueType = TValue;
	using Base::operator=;
	using Base::Base;
};

class ShaderRenderTarget final : public ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, 1>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, 1>;
	using Semantic = RenderTarget;
	using Base::operator=;
	using Base::Base;
};

class ShaderDepthTarget final : public ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, 1>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphTextureHandle, 1>;
	using Semantic = DepthTarget;
	using Base::operator=;
	using Base::Base;
};

template <typename TValue = void, std::size_t ArrayCount = 1>
class ShaderBuffer final : public ShaderParameterFields::ResourceArrayField<FrameGraphBufferHandle, ArrayCount>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphBufferHandle, ArrayCount>;
	using Semantic = ReadBuffer;
	using ValueType = TValue;
	using Base::operator=;
	using Base::Base;
};

template <typename TValue = void, std::size_t ArrayCount = 1>
class ShaderRWBuffer final : public ShaderParameterFields::ResourceArrayField<FrameGraphBufferHandle, ArrayCount>
{
  public:
	using Base = ShaderParameterFields::ResourceArrayField<FrameGraphBufferHandle, ArrayCount>;
	using Semantic = RWBuffer;
	using ValueType = TValue;
	using Base::operator=;
	using Base::Base;
};

template <typename TValue> class ShaderUniform final
{
  public:
	using Semantic = UniformData<TValue>;
	using ValueType = TValue;

	ShaderUniform() = default;

	ShaderUniform& operator=(const TValue& value) noexcept
	{
		m_value = value;
		m_isBound = true;
		return *this;
	}

	const TValue* GetValue() const noexcept { return m_isBound ? &m_value : nullptr; }

	bool IsBound() const noexcept { return m_isBound; }

	void Reset() noexcept
	{
		m_value = {};
		m_isBound = false;
	}

  private:
	TValue m_value = {};
	bool m_isBound = false;
};

class SPARKLE_RENDERER_API ShaderSamplerSet final
{
  public:
	using Semantic = SamplerSet;

	ShaderSamplerSet();

	ShaderSamplerSet& operator=(RhiSamplerDesc sampler) noexcept
	{
		m_sampler = sampler;
		m_isBound = true;
		return *this;
	}

	const RhiSamplerDesc& GetSampler() const noexcept { return m_sampler; }

	bool IsBound() const noexcept { return m_isBound; }

	void Reset() noexcept
	{
		m_sampler = {};
		m_isBound = false;
	}

  private:
	RhiSamplerDesc m_sampler = {};
	bool m_isBound = false;
};

template <typename T> struct ShaderParameterFieldTraits;

template <typename TValue, std::size_t ArrayCount> struct ShaderParameterFieldTraits<ShaderTexture2D<TValue, ArrayCount>>
{
	using Semantic = ReadTexture;
	static constexpr std::uint32_t FieldArrayCount = static_cast<std::uint32_t>(ArrayCount);
	static constexpr bool UsesGraphResource = true;
};

template <typename TValue, std::size_t ArrayCount> struct ShaderParameterFieldTraits<ShaderTexture3D<TValue, ArrayCount>>
{
	using Semantic = ReadTexture;
	static constexpr std::uint32_t FieldArrayCount = static_cast<std::uint32_t>(ArrayCount);
	static constexpr bool UsesGraphResource = true;
};

template <typename TValue, std::size_t ArrayCount> struct ShaderParameterFieldTraits<ShaderTextureCube<TValue, ArrayCount>>
{
	using Semantic = ReadTexture;
	static constexpr std::uint32_t FieldArrayCount = static_cast<std::uint32_t>(ArrayCount);
	static constexpr bool UsesGraphResource = true;
};

template <> struct ShaderParameterFieldTraits<ShaderTexture2DSRV>
{
	using Semantic = ReadTexture;
	static constexpr std::uint32_t FieldArrayCount = 1;
	static constexpr bool UsesGraphResource = false;
};

template <std::size_t ArrayCount> struct ShaderParameterFieldTraits<ShaderTexture2DTableSRV<ArrayCount>>
{
	using Semantic = ReadTexture;
	static constexpr std::uint32_t FieldArrayCount = static_cast<std::uint32_t>(ArrayCount);
	static constexpr bool UsesGraphResource = false;
};

template <> struct ShaderParameterFieldTraits<ShaderAccelerationStructure>
{
	using Semantic = AccelerationStructure;
	static constexpr std::uint32_t FieldArrayCount = 1;
	static constexpr bool UsesGraphResource = true;
};

template <typename TValue, std::size_t ArrayCount> struct ShaderParameterFieldTraits<ShaderRWTexture2D<TValue, ArrayCount>>
{
	using Semantic = RWTexture;
	static constexpr std::uint32_t FieldArrayCount = static_cast<std::uint32_t>(ArrayCount);
	static constexpr bool UsesGraphResource = true;
};

template <> struct ShaderParameterFieldTraits<ShaderRenderTarget>
{
	using Semantic = RenderTarget;
	static constexpr std::uint32_t FieldArrayCount = 1;
	static constexpr bool UsesGraphResource = true;
};

template <> struct ShaderParameterFieldTraits<ShaderDepthTarget>
{
	using Semantic = DepthTarget;
	static constexpr std::uint32_t FieldArrayCount = 1;
	static constexpr bool UsesGraphResource = true;
};

template <typename TValue, std::size_t ArrayCount> struct ShaderParameterFieldTraits<ShaderBuffer<TValue, ArrayCount>>
{
	using Semantic = ReadBuffer;
	static constexpr std::uint32_t FieldArrayCount = static_cast<std::uint32_t>(ArrayCount);
	static constexpr bool UsesGraphResource = true;
};

template <typename TValue, std::size_t ArrayCount> struct ShaderParameterFieldTraits<ShaderRWBuffer<TValue, ArrayCount>>
{
	using Semantic = RWBuffer;
	static constexpr std::uint32_t FieldArrayCount = static_cast<std::uint32_t>(ArrayCount);
	static constexpr bool UsesGraphResource = true;
};

template <typename TValue> struct ShaderParameterFieldTraits<ShaderUniform<TValue>>
{
	using Semantic = UniformData<TValue>;
	static constexpr std::uint32_t FieldArrayCount = 1;
	static constexpr bool UsesGraphResource = false;
};

template <> struct ShaderParameterFieldTraits<ShaderSamplerSet>
{
	using Semantic = SamplerSet;
	static constexpr std::uint32_t FieldArrayCount = 1;
	static constexpr bool UsesGraphResource = false;
};

template <typename T, typename = void> struct IsShaderParameterField : std::false_type
{
};

template <typename T> struct IsShaderParameterField<T, std::void_t<typename ShaderParameterFieldTraits<T>::Semantic>> : std::true_type
{
};

template <typename T> constexpr bool IsShaderParameterFieldV = IsShaderParameterField<T>::value;

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<ArrayCount == 1, bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderTexture2D<TValue, ArrayCount>& field)
{
	return parameterSet.SetTexture(name, field.GetValues()[0]);
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<(ArrayCount > 1), bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderTexture2D<TValue, ArrayCount>& field)
{
	return parameterSet.SetTextureArray(name, field.ToVector());
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<ArrayCount == 1, bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderTexture3D<TValue, ArrayCount>& field)
{
	return parameterSet.SetTexture(name, field.GetValues()[0]);
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<(ArrayCount > 1), bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderTexture3D<TValue, ArrayCount>& field)
{
	return parameterSet.SetTextureArray(name, field.ToVector());
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<ArrayCount == 1, bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderTextureCube<TValue, ArrayCount>& field)
{
	return parameterSet.SetTexture(name, field.GetValues()[0]);
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<(ArrayCount > 1), bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderTextureCube<TValue, ArrayCount>& field)
{
	return parameterSet.SetTextureArray(name, field.ToVector());
}

SPARKLE_RENDERER_API bool BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderTexture2DSRV& field);

template <std::size_t ArrayCount>
inline bool BindParameterField(PassParameterSet& parameterSet, const char* name, const ShaderTexture2DTableSRV<ArrayCount>& field)
{
	return parameterSet.SetShaderResourceView(name, field.GetDescriptorTable());
}

SPARKLE_RENDERER_API bool BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderAccelerationStructure& field);

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<ArrayCount == 1, bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderRWTexture2D<TValue, ArrayCount>& field)
{
	return parameterSet.SetTexture(name, field.GetValues()[0]);
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<(ArrayCount > 1), bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderRWTexture2D<TValue, ArrayCount>& field)
{
	return parameterSet.SetTextureArray(name, field.ToVector());
}

SPARKLE_RENDERER_API bool BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderRenderTarget& field);
SPARKLE_RENDERER_API bool BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderDepthTarget& field);

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<ArrayCount == 1, bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderBuffer<TValue, ArrayCount>& field)
{
	return parameterSet.SetBuffer(name, field.GetValues()[0]);
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<(ArrayCount > 1), bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderBuffer<TValue, ArrayCount>& field)
{
	return parameterSet.SetBufferArray(name, field.ToVector());
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<ArrayCount == 1, bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderRWBuffer<TValue, ArrayCount>& field)
{
	return parameterSet.SetBuffer(name, field.GetValues()[0]);
}

template <typename TValue, std::size_t ArrayCount>
std::enable_if_t<(ArrayCount > 1), bool> BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderRWBuffer<TValue, ArrayCount>& field)
{
	return parameterSet.SetBufferArray(name, field.ToVector());
}

template <typename TValue> bool BindParameterField(PassParameterSet& parameterSet, const char* name, const ShaderUniform<TValue>& field)
{
	if (!field.IsBound())
	{
		return false;
	}

	return parameterSet.SetUniformDataReference(name, *field.GetValue());
}

SPARKLE_RENDERER_API bool BindParameterField(
    PassParameterSet& parameterSet,
    const char* name,
    const ShaderSamplerSet& field);
