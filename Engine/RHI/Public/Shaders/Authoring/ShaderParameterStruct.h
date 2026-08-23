#pragma once

#include "../../RHIAPI.h"
#include "../../ShaderParameters/PassParameterLayout.h"
#include "../ShaderReflection.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

enum class ShaderParameterSamplerBindingPolicy : std::uint8_t
{
	None = 0,
	Shared,
	Unique,
};

struct ShaderParameterStructFieldDescriptor final
{
	// Shared default used when layout and shader names are identical.
	// Prefer LayoutName for engine binding identity and ShaderName for reflected HLSL symbols.
	std::string Name;
	std::string LayoutName;
	std::string ShaderName;
	CookedShaderResourceKind Kind = CookedShaderResourceKind::Unknown;
	CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
	ShaderParameterSemanticKind SemanticKind = ShaderParameterSemanticKind::ReadTexture;
	ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::None;
	ShaderParameterAccess Access = ShaderParameterAccess::None;
	ShaderStageVisibility Visibility = ShaderStageVisibility::None;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ValueSizeInBytes = 0;
	std::uint32_t ValueAlignmentInBytes = 0;
	bool Reflected = true;
	ShaderParameterSamplerBindingPolicy SamplerPolicy = ShaderParameterSamplerBindingPolicy::None;

	std::string_view GetLayoutName() const noexcept { return LayoutName.empty() ? Name : LayoutName; }

	std::string_view GetShaderName() const noexcept { return ShaderName.empty() ? Name : ShaderName; }
};

struct ShaderParameterStructDescriptor final
{
	std::string Name;
	std::vector<ShaderParameterStructFieldDescriptor> Fields;

	bool IsEmpty() const noexcept { return Fields.empty(); }
};

template <typename TParameters> class TShaderParameterStructRegistry final
{
  public:
	TShaderParameterStructRegistry() = delete;

	static void AddField(ShaderParameterStructFieldDescriptor field)
	{
		if (field.Name.empty())
		{
			field.Name = !field.ShaderName.empty() ? field.ShaderName : field.LayoutName;
		}
		if (field.LayoutName.empty())
		{
			field.LayoutName = field.Name;
		}
		if (field.ShaderName.empty())
		{
			field.ShaderName = field.Name;
		}

		std::vector<ShaderParameterStructFieldDescriptor>& fields = MutableFields();
		for (const ShaderParameterStructFieldDescriptor& existing : fields)
		{
			if (existing.GetLayoutName() == field.GetLayoutName())
			{
				return;
			}
		}

		fields.push_back(std::move(field));
	}

	static ShaderParameterStructDescriptor BuildDescriptor(std::string_view name)
	{
		ShaderParameterStructDescriptor descriptor;
		descriptor.Name.assign(name);
		descriptor.Fields = MutableFields();
		return descriptor;
	}

  private:
	static std::vector<ShaderParameterStructFieldDescriptor>& MutableFields()
	{
		static std::vector<ShaderParameterStructFieldDescriptor> fields;
		return fields;
	}
};

template <typename TParameters> class TShaderParameterFieldAutoRegister final
{
  public:
	TShaderParameterFieldAutoRegister(
	    std::string_view layoutName,
	    std::string_view shaderName,
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
		TShaderParameterStructRegistry<TParameters>::AddField(
		    ShaderParameterStructFieldDescriptor{
		        .Name = std::string(shaderName.empty() ? layoutName : shaderName),
		        .LayoutName = std::string(layoutName),
		        .ShaderName = std::string(shaderName.empty() ? layoutName : shaderName),
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

	TShaderParameterFieldAutoRegister(
	    std::string_view name,
	    CookedShaderResourceKind kind,
	    CookedShaderResourceDimension dimension,
	    std::uint32_t arrayCount,
	    std::uint32_t valueSizeInBytes,
	    std::uint32_t valueAlignmentInBytes) :
	    TShaderParameterFieldAutoRegister(
	        name,
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

template <typename TResource> struct TShaderParameterResourceTraits;

template <> struct TShaderParameterResourceTraits<Texture2D>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Texture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Texture2D;
};

template <> struct TShaderParameterResourceTraits<Texture3D>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Texture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Texture3D;
};

template <> struct TShaderParameterResourceTraits<TextureCube>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Texture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::TextureCube;
};

template <> struct TShaderParameterResourceTraits<RWTexture2D>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::RWTexture;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Texture2D;
};

template <> struct TShaderParameterResourceTraits<SamplerState>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::Sampler;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
};

template <> struct TShaderParameterResourceTraits<RaytracingAccelerationStructure>
{
	static constexpr CookedShaderResourceKind Kind = CookedShaderResourceKind::AccelerationStructure;
	static constexpr CookedShaderResourceDimension Dimension = CookedShaderResourceDimension::Unknown;
};

SPARKLE_RHI_API std::string BuildShaderParameterStructReport(const ShaderParameterStructDescriptor& descriptor);

