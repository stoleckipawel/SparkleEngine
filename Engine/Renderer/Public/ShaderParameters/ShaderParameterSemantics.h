#pragma once

#include <cstdint>
#include <type_traits>

enum class ShaderParameterSemanticKind : std::uint8_t
{
	ReadTexture,
	ReadBuffer,
	RWTexture,
	RWBuffer,
	RenderTarget,
	DepthTarget,
	UniformData,
	SamplerSet,
};

enum class ShaderParameterResourceDomain : std::uint8_t
{
	None,
	Texture,
	Buffer,
	Uniform,
	Sampler,
};

enum class ShaderParameterAccess : std::uint8_t
{
	None,
	Read,
	Write,
	ReadWrite,
};

struct ReadTexture
{
};

struct ReadBuffer
{
};

struct RWTexture
{
};

struct RWBuffer
{
};

struct RenderTarget
{
};

struct DepthTarget
{
};

struct SamplerSet
{
};

template <typename T> struct UniformData
{
	static_assert(std::is_trivially_copyable_v<T>, "UniformData requires a trivially copyable type.");
	static_assert(std::is_standard_layout_v<T>, "UniformData requires a standard-layout type.");

	using ValueType = T;
};

template <typename T> struct ShaderParameterSemanticTraits;

template <> struct ShaderParameterSemanticTraits<ReadTexture>
{
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::ReadTexture;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Texture;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::Read;
};

template <> struct ShaderParameterSemanticTraits<ReadBuffer>
{
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::ReadBuffer;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Buffer;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::Read;
};

template <> struct ShaderParameterSemanticTraits<RWTexture>
{
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::RWTexture;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Texture;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::ReadWrite;
};

template <> struct ShaderParameterSemanticTraits<RWBuffer>
{
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::RWBuffer;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Buffer;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::ReadWrite;
};

template <> struct ShaderParameterSemanticTraits<RenderTarget>
{
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::RenderTarget;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Texture;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::Write;
};

template <> struct ShaderParameterSemanticTraits<DepthTarget>
{
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::DepthTarget;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Texture;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::Write;
};

template <> struct ShaderParameterSemanticTraits<SamplerSet>
{
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::SamplerSet;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Sampler;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::None;
};

template <typename T> struct ShaderParameterSemanticTraits<UniformData<T>>
{
	using ValueType = T;
	static constexpr ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::UniformData;
	static constexpr ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::Uniform;
	static constexpr ShaderParameterAccess Access = ShaderParameterAccess::None;
};

template <typename T> struct IsShaderParameterSemantic : std::false_type
{
};

template <> struct IsShaderParameterSemantic<ReadTexture> : std::true_type
{
};

template <> struct IsShaderParameterSemantic<ReadBuffer> : std::true_type
{
};

template <> struct IsShaderParameterSemantic<RWTexture> : std::true_type
{
};

template <> struct IsShaderParameterSemantic<RWBuffer> : std::true_type
{
};

template <> struct IsShaderParameterSemantic<RenderTarget> : std::true_type
{
};

template <> struct IsShaderParameterSemantic<DepthTarget> : std::true_type
{
};

template <> struct IsShaderParameterSemantic<SamplerSet> : std::true_type
{
};

template <typename T> struct IsShaderParameterSemantic<UniformData<T>> : std::true_type
{
};

template <typename T> constexpr bool IsShaderParameterSemanticV = IsShaderParameterSemantic<T>::value;