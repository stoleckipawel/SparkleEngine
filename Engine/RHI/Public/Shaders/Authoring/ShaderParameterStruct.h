#pragma once

#include "../../RHIAPI.h"
#include "../../ShaderParameters/PassParameterLayout.h"
#include "../ShaderReflection.h"
#include "Core/Public/Hash/HashUtils.h"

#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <DirectXMath.h>
#include <algorithm>

enum class ShaderParameterSamplerBindingPolicy : std::uint8_t
{
	None = 0,
	Shared,
	Unique,
};

struct ShaderParameterStructFieldDescriptor final
{
	std::string Name;
	CookedShaderResourceKind Kind = CookedShaderResourceKind::Unknown;
	CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
	ShaderParameterSemanticKind SemanticKind = ShaderParameterSemanticKind::ReadTexture;
	ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::None;
	ShaderParameterAccess Access = ShaderParameterAccess::None;
	ShaderStageVisibility Visibility = ShaderStageVisibility::None;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ValueSizeInBytes = 0;
	std::uint32_t ValueAlignmentInBytes = 0;
	std::uint64_t ValueLayoutHash = 0;
	bool Reflected = true;
	ShaderParameterSamplerBindingPolicy SamplerPolicy = ShaderParameterSamplerBindingPolicy::None;
};

struct ShaderParameterValueDescriptor final
{
	std::string Name;
	std::string HlslType;
	std::uint32_t OffsetInBytes = 0;
	std::uint32_t SizeInBytes = 0;
};

template <typename T> constexpr const char* GetShaderParameterValueType()
{
	if constexpr (std::is_same_v<T, float>)
	{
		return "float";
	}
	else if constexpr (std::is_same_v<T, std::uint32_t>)
	{
		return "uint";
	}
	else if constexpr (std::is_same_v<T, std::int32_t>)
	{
		return "int";
	}
	else if constexpr (std::is_same_v<T, DirectX::XMFLOAT2>)
	{
		return "float2";
	}
	else if constexpr (std::is_same_v<T, DirectX::XMFLOAT3>)
	{
		return "float3";
	}
	else if constexpr (std::is_same_v<T, DirectX::XMFLOAT4>)
	{
		return "float4";
	}
	else if constexpr (std::is_same_v<T, DirectX::XMFLOAT4X4>)
	{
		return "row_major float4x4";
	}
	else
	{
		static_assert(sizeof(T) == 0, "Unsupported shader parameter value type.");
	}
}

struct ShaderParameterStructDescriptor final
{
	std::string Name;
	std::vector<ShaderParameterStructFieldDescriptor> Fields;
	std::vector<ShaderParameterValueDescriptor> Values;
	std::string AutoParametersName;

	bool IsEmpty() const noexcept { return Fields.empty(); }
};

template <typename TParameters> class ShaderParameterDescriptorRegistry final
{
public:
	ShaderParameterDescriptorRegistry() = delete;

	static void AddField(ShaderParameterStructFieldDescriptor field)
	{
		std::vector<ShaderParameterStructFieldDescriptor>& fields = MutableFields();
		for (const ShaderParameterStructFieldDescriptor& existing : fields)
		{
			if (existing.Name == field.Name)
			{
				throw std::logic_error("Duplicate shader parameter field registration: " + field.Name);
			}
		}

		fields.push_back(std::move(field));
	}

	static ShaderParameterStructDescriptor BuildDescriptor(std::string_view name)
	{
		ShaderParameterStructDescriptor descriptor;
		descriptor.Name.assign(name);
		descriptor.Fields = MutableFields();
		descriptor.Values = MutableValues();
		if (!descriptor.Values.empty())
		{
			descriptor.AutoParametersName = "AutoParameters_" + std::string(name);
			std::uint32_t size = 0;
			std::uint64_t layoutHash = Hash::kFnv64OffsetBasis;
			for (auto& value : descriptor.Values)
			{
				if (value.SizeInBytes > 16u || size % 16u + value.SizeInBytes > 16u)
				{
					size = (size + 15u) & ~15u;
				}
				value.OffsetInBytes = size;
				size += value.SizeInBytes;
				layoutHash = Hash::ContinueFnv1a64(layoutHash, value.Name.data(), value.Name.size());
				layoutHash = Hash::ContinueFnv1a64(layoutHash, value.HlslType.data(), value.HlslType.size());
				layoutHash = Hash::ContinueFnv1a64Value(layoutHash, value.OffsetInBytes);
				layoutHash = Hash::ContinueFnv1a64Value(layoutHash, value.SizeInBytes);
			}
			descriptor.Fields.push_back(
			    ShaderParameterStructFieldDescriptor{
			        .Name = descriptor.AutoParametersName,
			        .Kind = CookedShaderResourceKind::ConstantBuffer,
			        .Dimension = CookedShaderResourceDimension::Buffer,
			        .SemanticKind = ShaderParameterSemanticKind::UniformData,
			        .ResourceDomain = ShaderParameterResourceDomain::Uniform,
			        .ValueSizeInBytes = (size + 15u) & ~15u,
			        .ValueAlignmentInBytes = 16u,
			        .ValueLayoutHash = Hash::FinalizeFnv1a64(layoutHash)});
		}
		return descriptor;
	}

	template <typename T> static void AddValue(std::string_view name)
	{
		auto& values = MutableValues();
		if (std::ranges::any_of(values, [name](const auto& value) { return value.Name == name; }))
		{
			throw std::logic_error("Duplicate shader parameter value: " + std::string(name));
		}
		values.push_back({std::string(name), GetShaderParameterValueType<T>(), 0u, static_cast<std::uint32_t>(sizeof(T))});
	}

private:
	static std::vector<ShaderParameterValueDescriptor>& MutableValues()
	{
		static std::vector<ShaderParameterValueDescriptor> values;
		return values;
	}
	static std::vector<ShaderParameterStructFieldDescriptor>& MutableFields()
	{
		static std::vector<ShaderParameterStructFieldDescriptor> fields;
		return fields;
	}
};

template <typename TParameters> class ShaderParameterDescriptorAutoRegister final
{
public:
	ShaderParameterDescriptorAutoRegister(
	    std::string_view name,
	    CookedShaderResourceKind kind,
	    CookedShaderResourceDimension dimension,
	    ShaderParameterSemanticKind semanticKind,
	    ShaderParameterResourceDomain resourceDomain,
	    ShaderParameterAccess access,
	    ShaderStageVisibility visibility,
	    std::uint32_t arrayCount,
	    std::uint32_t valueSizeInBytes,
	    std::uint32_t valueAlignmentInBytes,
	    bool reflected,
	    ShaderParameterSamplerBindingPolicy samplerPolicy = ShaderParameterSamplerBindingPolicy::None)
	{
		ShaderParameterDescriptorRegistry<TParameters>::AddField(
		    ShaderParameterStructFieldDescriptor{
		        .Name = std::string(name),
		        .Kind = kind,
		        .Dimension = dimension,
		        .SemanticKind = semanticKind,
		        .ResourceDomain = resourceDomain,
		        .Access = access,
		        .Visibility = visibility,
		        .ArrayCount = arrayCount,
		        .ValueSizeInBytes = valueSizeInBytes,
		        .ValueAlignmentInBytes = valueAlignmentInBytes,
		        .Reflected = reflected,
		        .SamplerPolicy = samplerPolicy,
		    });
	}

	ShaderParameterDescriptorAutoRegister(
	    std::string_view name,
	    CookedShaderResourceKind kind,
	    CookedShaderResourceDimension dimension,
	    std::uint32_t arrayCount,
	    std::uint32_t valueSizeInBytes,
	    std::uint32_t valueAlignmentInBytes) :
	    ShaderParameterDescriptorAutoRegister(
	        name,
	        kind,
	        dimension,
	        GetShaderParameterSemanticKind(kind),
	        GetShaderParameterResourceDomain(kind),
	        GetShaderParameterAccess(kind),
	        ShaderStageVisibility::None,
	        arrayCount,
	        valueSizeInBytes,
	        valueAlignmentInBytes,
	        true)
	{
	}

private:
	static constexpr ShaderParameterSemanticKind GetShaderParameterSemanticKind(CookedShaderResourceKind kind) noexcept
	{
		switch (kind)
		{
			case CookedShaderResourceKind::ConstantBuffer:
			case CookedShaderResourceKind::PushConstantBlock:
				return ShaderParameterSemanticKind::UniformData;
			case CookedShaderResourceKind::Texture:
				return ShaderParameterSemanticKind::ReadTexture;
			case CookedShaderResourceKind::StructuredBuffer:
			case CookedShaderResourceKind::ByteAddressBuffer:
			case CookedShaderResourceKind::TypedBuffer:
				return ShaderParameterSemanticKind::ReadBuffer;
			case CookedShaderResourceKind::RWTexture:
				return ShaderParameterSemanticKind::RWTexture;
			case CookedShaderResourceKind::RWStructuredBuffer:
			case CookedShaderResourceKind::RWByteAddressBuffer:
			case CookedShaderResourceKind::RWTypedBuffer:
				return ShaderParameterSemanticKind::RWBuffer;
			case CookedShaderResourceKind::Sampler:
				return ShaderParameterSemanticKind::SamplerSet;
			case CookedShaderResourceKind::AccelerationStructure:
				return ShaderParameterSemanticKind::AccelerationStructure;
			case CookedShaderResourceKind::Unknown:
			default:
				return ShaderParameterSemanticKind::ReadTexture;
		}
	}

	static constexpr ShaderParameterResourceDomain GetShaderParameterResourceDomain(CookedShaderResourceKind kind) noexcept
	{
		switch (kind)
		{
			case CookedShaderResourceKind::ConstantBuffer:
			case CookedShaderResourceKind::PushConstantBlock:
				return ShaderParameterResourceDomain::Uniform;
			case CookedShaderResourceKind::Texture:
			case CookedShaderResourceKind::RWTexture:
				return ShaderParameterResourceDomain::Texture;
			case CookedShaderResourceKind::StructuredBuffer:
			case CookedShaderResourceKind::ByteAddressBuffer:
			case CookedShaderResourceKind::TypedBuffer:
			case CookedShaderResourceKind::RWStructuredBuffer:
			case CookedShaderResourceKind::RWByteAddressBuffer:
			case CookedShaderResourceKind::RWTypedBuffer:
				return ShaderParameterResourceDomain::Buffer;
			case CookedShaderResourceKind::Sampler:
				return ShaderParameterResourceDomain::Sampler;
			case CookedShaderResourceKind::AccelerationStructure:
				return ShaderParameterResourceDomain::AccelerationStructure;
			case CookedShaderResourceKind::Unknown:
			default:
				return ShaderParameterResourceDomain::None;
		}
	}

	static constexpr ShaderParameterAccess GetShaderParameterAccess(CookedShaderResourceKind kind) noexcept
	{
		switch (kind)
		{
			case CookedShaderResourceKind::Texture:
			case CookedShaderResourceKind::StructuredBuffer:
			case CookedShaderResourceKind::ByteAddressBuffer:
			case CookedShaderResourceKind::TypedBuffer:
			case CookedShaderResourceKind::AccelerationStructure:
				return ShaderParameterAccess::Read;
			case CookedShaderResourceKind::RWTexture:
			case CookedShaderResourceKind::RWStructuredBuffer:
			case CookedShaderResourceKind::RWByteAddressBuffer:
			case CookedShaderResourceKind::RWTypedBuffer:
				return ShaderParameterAccess::ReadWrite;
			case CookedShaderResourceKind::ConstantBuffer:
			case CookedShaderResourceKind::PushConstantBlock:
			case CookedShaderResourceKind::Sampler:
			case CookedShaderResourceKind::Unknown:
			default:
				return ShaderParameterAccess::None;
		}
	}
};

struct RaytracingAccelerationStructure final
{
};

struct Texture2D final
{
};

struct Texture3D final
{
};

struct TextureCube final
{
};

struct RWTexture2D final
{
};

struct SamplerState final
{
};

template <typename TValue = void> struct StructuredBuffer final
{
	using ValueType = TValue;
};

template <typename TValue = void> struct RWStructuredBuffer final
{
	using ValueType = TValue;
};

struct ShaderRenderTargetParameter final
{
};

struct ShaderDepthTargetParameter final
{
};

template <typename TResource> struct ShaderParameterResourceTraits;

template <> struct ShaderParameterResourceTraits<Texture2D>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Texture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Texture2D;
};

template <> struct ShaderParameterResourceTraits<Texture3D>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Texture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Texture3D;
};

template <> struct ShaderParameterResourceTraits<TextureCube>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Texture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::TextureCube;
};

template <> struct ShaderParameterResourceTraits<RWTexture2D>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::RWTexture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Texture2D;
};

template <> struct ShaderParameterResourceTraits<SamplerState>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Sampler;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
};

template <> struct ShaderParameterResourceTraits<RaytracingAccelerationStructure>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::AccelerationStructure;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
};

SPARKLE_RHI_API std::string BuildShaderParameterStructReport(const ShaderParameterStructDescriptor& descriptor);
