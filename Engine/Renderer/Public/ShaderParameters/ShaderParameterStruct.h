#pragma once

#include "ShaderParameterStructBuilder.h"
#include "RHI/Public/Shaders/Authoring/ShaderParameterStruct.h"

#include <cstddef>
#include <cstdint>

template <typename TResource, std::size_t ArrayCount = 1> struct ShaderTextureSRVField;

template <std::size_t ArrayCount> struct ShaderTextureSRVField<Texture2D, ArrayCount>
{
	using Type = ShaderTexture2D<void, ArrayCount>;
};

template <std::size_t ArrayCount> struct ShaderTextureSRVField<Texture3D, ArrayCount>
{
	using Type = ShaderTexture3D<void, ArrayCount>;
};

template <std::size_t ArrayCount> struct ShaderTextureSRVField<TextureCube, ArrayCount>
{
	using Type = ShaderTextureCube<void, ArrayCount>;
};

template <typename TResource> struct ShaderTextureUAVField;

template <> struct ShaderTextureUAVField<RWTexture2D>
{
	using Type = ShaderRWTexture2D<>;
};

template <typename TResource> struct ShaderBufferSRVField;

template <typename TValue> struct ShaderBufferSRVField<StructuredBuffer<TValue>>
{
	using Type = ShaderBuffer<TValue>;
};

template <typename TResource> struct ShaderBufferUAVField;

template <typename TValue> struct ShaderBufferUAVField<RWStructuredBuffer<TValue>>
{
	using Type = ShaderRWBuffer<TValue>;
};

#define BEGIN_SHADER_PARAMETER_STRUCT(StructName, Prefix)                                                        \
	struct StructName                                                                                            \
	{                                                                                                            \
		using ThisShaderParameterStruct = StructName;                                                            \
		static ::ShaderParameterStructDescriptor GetShaderParameterStructDescriptor()                            \
		{                                                                                                        \
			return ::ShaderParameterDescriptorRegistry<ThisShaderParameterStruct>::BuildDescriptor(#StructName); \
		}

#if defined(SPARKLE_SHADER_CONTRACTS_ONLY)
#define SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, Visibility)
#define SPARKLE_REGISTER_EXTERNAL_GRAPH_SHADER_PARAMETER(Name, Visibility)
#else
#define SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, Visibility)                                                                       \
	inline static const ::ShaderParameterFieldAutoRegister<ThisShaderParameterStruct, decltype(Name)> AutoRegisterGraphParameter_##Name \
	{                                                                                                                                   \
		#Name, &ThisShaderParameterStruct::Name, Visibility                                                                             \
	}

#define SPARKLE_REGISTER_EXTERNAL_GRAPH_SHADER_PARAMETER(Name, Visibility)                                                              \
	inline static const ::ShaderParameterFieldAutoRegister<ThisShaderParameterStruct, decltype(Name)> AutoRegisterGraphParameter_##Name \
	{                                                                                                                                   \
		#Name, &ThisShaderParameterStruct::Name, Visibility, false                                                                      \
	}
#endif

#define SHADER_PARAMETER_CBUFFER(UniformType, Name)                                                                      \
	::ShaderUniform<UniformType> Name{};                                                                                 \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::CookedShaderResourceKind::ConstantBuffer,                                                                      \
	    ::CookedShaderResourceDimension::Buffer,                                                                         \
	    ::ShaderParameterSemanticKind::UniformData,                                                                      \
	    ::ShaderParameterResourceDomain::Uniform,                                                                        \
	    ::ShaderParameterAccess::None,                                                                                   \
	    ::ShaderStageVisibility::None,                                                                                   \
	    1u,                                                                                                              \
	    static_cast<std::uint32_t>(sizeof(UniformType)),                                                                 \
	    static_cast<std::uint32_t>(alignof(UniformType)),                                                                \
	    true};                                                                                                           \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_TEXTURE_SRV(ResourceType, Name)                                                                 \
	typename ::ShaderTextureSRVField<::ResourceType>::Type Name{};                                                       \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Kind,                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Dimension,                                                      \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u};                                                                                                             \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_EXTERNAL_TEXTURE_SRV(ResourceType, Name)                                                        \
	::ShaderTexture2DSRV Name{};                                                                                         \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Kind,                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Dimension,                                                      \
	    ::ShaderParameterSemanticKind::ReadTexture,                                                                      \
	    ::ShaderParameterResourceDomain::Texture,                                                                        \
	    ::ShaderParameterAccess::Read,                                                                                   \
	    ::ShaderStageVisibility::None,                                                                                   \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u,                                                                                                              \
	    true};                                                                                                           \
	SPARKLE_REGISTER_EXTERNAL_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_TEXTURE_SRV_ARRAY(ResourceType, Name, Count)                                                    \
	::ShaderTexture2DTableSRV<Count> Name{};                                                                             \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Kind,                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Dimension,                                                      \
	    ::ShaderParameterSemanticKind::ReadTexture,                                                                      \
	    ::ShaderParameterResourceDomain::Texture,                                                                        \
	    ::ShaderParameterAccess::Read,                                                                                   \
	    ::ShaderStageVisibility::None,                                                                                   \
	    static_cast<std::uint32_t>(Count),                                                                               \
	    0u,                                                                                                              \
	    0u,                                                                                                              \
	    true};                                                                                                           \
	SPARKLE_REGISTER_EXTERNAL_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_TEXTURE_UAV(ResourceType, Name)                                                                 \
	typename ::ShaderTextureUAVField<::ResourceType>::Type Name{};                                                       \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Kind,                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Dimension,                                                      \
	    ::ShaderParameterSemanticKind::RWTexture,                                                                        \
	    ::ShaderParameterResourceDomain::Texture,                                                                        \
	    ::ShaderParameterAccess::ReadWrite,                                                                              \
	    ::ShaderStageVisibility::None,                                                                                   \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u,                                                                                                              \
	    true};                                                                                                           \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_BUFFER_SRV(ValueType, Name)                                                                     \
	typename ::ShaderBufferSRVField<::StructuredBuffer<ValueType>>::Type Name{};                                         \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::CookedShaderResourceKind::StructuredBuffer,                                                                    \
	    ::CookedShaderResourceDimension::Buffer,                                                                         \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u};                                                                                                             \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_EXTERNAL_BUFFER_SRV(ValueType, Name)                                                            \
	typename ::ShaderBufferSRVField<::StructuredBuffer<ValueType>>::Type Name{};                                         \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::CookedShaderResourceKind::StructuredBuffer,                                                                    \
	    ::CookedShaderResourceDimension::Buffer,                                                                         \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u};                                                                                                             \
	SPARKLE_REGISTER_EXTERNAL_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_BUFFER_UAV(ValueType, Name)                                                                     \
	typename ::ShaderBufferUAVField<::RWStructuredBuffer<ValueType>>::Type Name{};                                       \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::CookedShaderResourceKind::RWStructuredBuffer,                                                                  \
	    ::CookedShaderResourceDimension::Buffer,                                                                         \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u};                                                                                                             \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_SAMPLER(ResourceType, Name)                                                                     \
	::ShaderSamplerSet Name{};                                                                                           \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Kind,                                                           \
	    ::ShaderParameterResourceTraits<::ResourceType>::Dimension,                                                      \
	    ::ShaderParameterSemanticKind::SamplerSet,                                                                       \
	    ::ShaderParameterResourceDomain::Sampler,                                                                        \
	    ::ShaderParameterAccess::None,                                                                                   \
	    ::ShaderStageVisibility::None,                                                                                   \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u,                                                                                                              \
	    true,                                                                                                            \
	    ::ShaderParameterSamplerBindingPolicy::Unique};                                                                  \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_SHARED_SAMPLER(Name)                                                                            \
	::ShaderSamplerSet Name{};                                                                                           \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::CookedShaderResourceKind::Sampler,                                                                             \
	    ::CookedShaderResourceDimension::Unknown,                                                                        \
	    ::ShaderParameterSemanticKind::SamplerSet,                                                                       \
	    ::ShaderParameterResourceDomain::Sampler,                                                                        \
	    ::ShaderParameterAccess::None,                                                                                   \
	    ::ShaderStageVisibility::None,                                                                                   \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u,                                                                                                              \
	    true,                                                                                                            \
	    ::ShaderParameterSamplerBindingPolicy::Shared};                                                                  \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_ACCELERATION_STRUCTURE(Name)                                                                    \
	::ShaderAccelerationStructure Name{};                                                                                \
	inline static const ::ShaderParameterDescriptorAutoRegister<ThisShaderParameterStruct> AutoRegisterParameter_##Name{ \
	    #Name,                                                                                                           \
	    ::CookedShaderResourceKind::AccelerationStructure,                                                               \
	    ::CookedShaderResourceDimension::Unknown,                                                                        \
	    ::ShaderParameterSemanticKind::AccelerationStructure,                                                            \
	    ::ShaderParameterResourceDomain::AccelerationStructure,                                                          \
	    ::ShaderParameterAccess::Read,                                                                                   \
	    ::ShaderStageVisibility::None,                                                                                   \
	    1u,                                                                                                              \
	    0u,                                                                                                              \
	    0u,                                                                                                              \
	    true};                                                                                                           \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::All);

#define SHADER_PARAMETER_RENDER_TARGET(Name) \
	::ShaderRenderTarget Name{};             \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::AllGraphics);

#define SHADER_PARAMETER_DEPTH_TARGET(Name) \
	::ShaderDepthTarget Name{};             \
	SPARKLE_REGISTER_GRAPH_SHADER_PARAMETER(Name, ::ShaderStageVisibility::AllGraphics);

#define END_SHADER_PARAMETER_STRUCT() \
	}                                 \
	;
