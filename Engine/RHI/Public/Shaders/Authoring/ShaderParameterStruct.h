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
	// Compatibility name used by the current reflection verifier. New code should
	// prefer LayoutName for engine binding identity and ShaderName for reflected
	// HLSL symbols.
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

#define BEGIN_SHADER_PARAMETER_STRUCT(StructName, Prefix)                                                     \
	struct StructName                                                                                         \
	{                                                                                                         \
		using ThisShaderParameterStruct = StructName;                                                         \
		static ::ShaderParameterStructDescriptor GetShaderParameterStructDescriptor()                         \
		{                                                                                                     \
			return ::TShaderParameterStructRegistry<ThisShaderParameterStruct>::BuildDescriptor(#StructName); \
		}

#define SHADER_PARAMETER(Type, Name)                                                                                 \
	Type Name{};                                                                                                     \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    ::CookedShaderResourceKind::ConstantBuffer,                                                                  \
	    ::CookedShaderResourceDimension::Buffer,                                                                     \
	    1u,                                                                                                          \
	    static_cast<std::uint32_t>(sizeof(Type)),                                                                    \
	    static_cast<std::uint32_t>(alignof(Type))};

#define SHADER_PARAMETER_CBUFFER(LayoutName, Type)                                                                         \
	Type LayoutName{};                                                                                                     \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##LayoutName{ \
	    #LayoutName,                                                                                                       \
	    #Type,                                                                                                             \
	    ::CookedShaderResourceKind::ConstantBuffer,                                                                        \
	    ::CookedShaderResourceDimension::Buffer,                                                                           \
	    ::ShaderParameterSemanticKind::UniformData,                                                                        \
	    ::ShaderParameterResourceDomain::Uniform,                                                                          \
	    ::ShaderParameterAccess::None,                                                                                     \
	    ::ShaderStageVisibility::None,                                                                                     \
	    1u,                                                                                                                \
	    static_cast<std::uint32_t>(sizeof(Type)),                                                                          \
	    static_cast<std::uint32_t>(alignof(Type)),                                                                         \
	    true};

#define SHADER_PARAMETER_CBUFFER_NAMED(LayoutName, ShaderName, Type)                                                       \
	Type LayoutName{};                                                                                                     \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##LayoutName{ \
	    #LayoutName,                                                                                                       \
	    #ShaderName,                                                                                                       \
	    ::CookedShaderResourceKind::ConstantBuffer,                                                                        \
	    ::CookedShaderResourceDimension::Buffer,                                                                           \
	    ::ShaderParameterSemanticKind::UniformData,                                                                        \
	    ::ShaderParameterResourceDomain::Uniform,                                                                          \
	    ::ShaderParameterAccess::None,                                                                                     \
	    ::ShaderStageVisibility::None,                                                                                     \
	    1u,                                                                                                                \
	    static_cast<std::uint32_t>(sizeof(Type)),                                                                          \
	    static_cast<std::uint32_t>(alignof(Type)),                                                                         \
	    true};

#define SHADER_PARAMETER_TEXTURE(Type, Name)                                                                         \
	::Type Name{};                                                                                                   \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    ::TShaderParameterResourceTraits<::Type>::Kind,                                                              \
	    ::TShaderParameterResourceTraits<::Type>::Dimension,                                                         \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u};

#define SHADER_PARAMETER_TEXTURE_NAMED(Type, LayoutName, ShaderName)                                                       \
	::Type LayoutName{};                                                                                                   \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##LayoutName{ \
	    #LayoutName,                                                                                                       \
	    #ShaderName,                                                                                                       \
	    ::TShaderParameterResourceTraits<::Type>::Kind,                                                                    \
	    ::TShaderParameterResourceTraits<::Type>::Dimension,                                                               \
	    ::ShaderParameterSemanticKind::ReadTexture,                                                                        \
	    ::ShaderParameterResourceDomain::Texture,                                                                          \
	    ::ShaderParameterAccess::Read,                                                                                     \
	    ::ShaderStageVisibility::None,                                                                                     \
	    1u,                                                                                                                \
	    0u,                                                                                                                \
	    0u,                                                                                                                \
	    true};

#define SHADER_PARAMETER_TEXTURE_ARRAY(Type, Name, Count)                                                            \
	::Type Name[Count]{};                                                                                            \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    #Name,                                                                                                       \
	    ::TShaderParameterResourceTraits<::Type>::Kind,                                                              \
	    ::TShaderParameterResourceTraits<::Type>::Dimension,                                                         \
	    ::ShaderParameterSemanticKind::ReadTexture,                                                                  \
	    ::ShaderParameterResourceDomain::Texture,                                                                    \
	    ::ShaderParameterAccess::Read,                                                                               \
	    ::ShaderStageVisibility::None,                                                                               \
	    static_cast<std::uint32_t>(Count),                                                                           \
	    0u,                                                                                                          \
	    0u,                                                                                                          \
	    true};

#define SHADER_PARAMETER_UAV(Type, Name)                                                                             \
	::Type Name{};                                                                                                   \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    #Name,                                                                                                       \
	    ::TShaderParameterResourceTraits<::Type>::Kind,                                                              \
	    ::TShaderParameterResourceTraits<::Type>::Dimension,                                                         \
	    ::ShaderParameterSemanticKind::RWTexture,                                                                    \
	    ::ShaderParameterResourceDomain::Texture,                                                                    \
	    ::ShaderParameterAccess::ReadWrite,                                                                          \
	    ::ShaderStageVisibility::None,                                                                               \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u,                                                                                                          \
	    true};

#define SHADER_PARAMETER_UAV_NAMED(Type, LayoutName, ShaderName)                                                           \
	::Type LayoutName{};                                                                                                   \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##LayoutName{ \
	    #LayoutName,                                                                                                       \
	    #ShaderName,                                                                                                       \
	    ::TShaderParameterResourceTraits<::Type>::Kind,                                                                    \
	    ::TShaderParameterResourceTraits<::Type>::Dimension,                                                               \
	    ::ShaderParameterSemanticKind::RWTexture,                                                                          \
	    ::ShaderParameterResourceDomain::Texture,                                                                          \
	    ::ShaderParameterAccess::ReadWrite,                                                                                \
	    ::ShaderStageVisibility::None,                                                                                     \
	    1u,                                                                                                                \
	    0u,                                                                                                                \
	    0u,                                                                                                                \
	    true};

#define SHADER_PARAMETER_SAMPLER(Type, Name)                                           \
	::Type Name{};                                                                     \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> \
	    AutoRegisterParameter_##Name{#Name, ::CookedShaderResourceKind::Sampler, ::CookedShaderResourceDimension::Unknown, 1u, 0u, 0u};

#define SHADER_PARAMETER_SHARED_SAMPLER(Name)                                                                        \
	::SamplerState Name{};                                                                                           \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    #Name,                                                                                                       \
	    ::CookedShaderResourceKind::Sampler,                                                                         \
	    ::CookedShaderResourceDimension::Unknown,                                                                    \
	    ::ShaderParameterSemanticKind::SamplerSet,                                                                   \
	    ::ShaderParameterResourceDomain::Sampler,                                                                    \
	    ::ShaderParameterAccess::None,                                                                               \
	    ::ShaderStageVisibility::None,                                                                               \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u,                                                                                                          \
	    true,                                                                                                        \
	    ::ShaderParameterSamplerBindingPolicy::Shared};

#define SHADER_PARAMETER_ACCELERATION_STRUCTURE(Name)                                                                \
	::RaytracingAccelerationStructure Name{};                                                                        \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    #Name,                                                                                                       \
	    ::CookedShaderResourceKind::AccelerationStructure,                                                           \
	    ::CookedShaderResourceDimension::Unknown,                                                                    \
	    ::ShaderParameterSemanticKind::AccelerationStructure,                                                        \
	    ::ShaderParameterResourceDomain::AccelerationStructure,                                                      \
	    ::ShaderParameterAccess::Read,                                                                               \
	    ::ShaderStageVisibility::None,                                                                               \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u,                                                                                                          \
	    true};

#define SHADER_PARAMETER_UNIQUE_SAMPLER(Type, Name)                                                                  \
	::Type Name{};                                                                                                   \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    #Name,                                                                                                       \
	    ::TShaderParameterResourceTraits<::Type>::Kind,                                                              \
	    ::TShaderParameterResourceTraits<::Type>::Dimension,                                                         \
	    ::ShaderParameterSemanticKind::SamplerSet,                                                                   \
	    ::ShaderParameterResourceDomain::Sampler,                                                                    \
	    ::ShaderParameterAccess::None,                                                                               \
	    ::ShaderStageVisibility::None,                                                                               \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u,                                                                                                          \
	    true,                                                                                                        \
	    ::ShaderParameterSamplerBindingPolicy::Unique};

#define SHADER_PARAMETER_RENDER_TARGET(Name)                                                                         \
	::ShaderRenderTargetParameter Name{};                                                                            \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    "",                                                                                                          \
	    ::CookedShaderResourceKind::Unknown,                                                                         \
	    ::CookedShaderResourceDimension::Texture2D,                                                                  \
	    ::ShaderParameterSemanticKind::RenderTarget,                                                                 \
	    ::ShaderParameterResourceDomain::Texture,                                                                    \
	    ::ShaderParameterAccess::Write,                                                                              \
	    ::ShaderStageVisibility::AllGraphics,                                                                        \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u,                                                                                                          \
	    false};

#define SHADER_PARAMETER_DEPTH_TARGET(Name)                                                                          \
	::ShaderDepthTargetParameter Name{};                                                                             \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    "",                                                                                                          \
	    ::CookedShaderResourceKind::Unknown,                                                                         \
	    ::CookedShaderResourceDimension::Texture2D,                                                                  \
	    ::ShaderParameterSemanticKind::DepthTarget,                                                                  \
	    ::ShaderParameterResourceDomain::Texture,                                                                    \
	    ::ShaderParameterAccess::Write,                                                                              \
	    ::ShaderStageVisibility::AllGraphics,                                                                        \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u,                                                                                                          \
	    false};

#define SHADER_PARAMETER_RDG_BUFFER_SRV(Type, Name)                                                                  \
	::StructuredBuffer<Type> Name{};                                                                                 \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    ::CookedShaderResourceKind::StructuredBuffer,                                                                \
	    ::CookedShaderResourceDimension::Buffer,                                                                     \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u};

#define SHADER_PARAMETER_RDG_BUFFER_UAV(Type, Name)                                                                  \
	::RWStructuredBuffer<Type> Name{};                                                                               \
	inline static const ::TShaderParameterFieldAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                       \
	    ::CookedShaderResourceKind::RWStructuredBuffer,                                                              \
	    ::CookedShaderResourceDimension::Buffer,                                                                     \
	    1u,                                                                                                          \
	    0u,                                                                                                          \
	    0u};

#define SHADER_PARAMETER_RDG_TEXTURE_SRV(Type, Name) SHADER_PARAMETER_TEXTURE(Type, Name)

#define SHADER_PARAMETER_RDG_TEXTURE_UAV(Type, Name) SHADER_PARAMETER_UAV(Type, Name)

#define END_SHADER_PARAMETER_STRUCT() \
	}                                 \
	;