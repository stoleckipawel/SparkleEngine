#pragma once

#include "../RHIAPI.h"
#include "ShaderParameterSemantics.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

enum class ShaderStageVisibility : std::uint8_t
{
	None = 0,
	Vertex = 1 << 0,
	Pixel = 1 << 1,
	Compute = 1 << 2,
	AllGraphics = (1 << 0) | (1 << 1),
	All = (1 << 0) | (1 << 1) | (1 << 2),
};

SPARKLE_RHI_API ShaderStageVisibility operator|(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept;
SPARKLE_RHI_API ShaderStageVisibility operator&(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept;
SPARKLE_RHI_API ShaderStageVisibility& operator|=(ShaderStageVisibility& lhs, ShaderStageVisibility rhs) noexcept;
SPARKLE_RHI_API bool HasAnyShaderStageVisibility(ShaderStageVisibility value, ShaderStageVisibility flags) noexcept;

struct PassParameterDesc
{
	std::string Name;
	ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::ReadTexture;
	ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::None;
	ShaderParameterAccess Access = ShaderParameterAccess::None;
	ShaderStageVisibility Visibility = ShaderStageVisibility::All;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ValueSizeInBytes = 0;

	bool IsArray() const noexcept { return ArrayCount > 1; }

	bool IsUniformData() const noexcept { return Kind == ShaderParameterSemanticKind::UniformData; }
};

template <typename T> struct PassParameterValueSize
{
	static constexpr std::uint32_t Value = 0;
};

template <typename T> struct PassParameterValueSize<UniformData<T>>
{
	static constexpr std::uint32_t Value = static_cast<std::uint32_t>(sizeof(T));
};

class SPARKLE_RHI_API PassParameterLayout final
{
public:
	PassParameterLayout();
	explicit PassParameterLayout(const char* debugName);

	const std::string& GetDebugName() const noexcept { return m_debugName; }

	void SetDebugName(const char* debugName) { m_debugName = debugName != nullptr ? debugName : ""; }

	bool IsEmpty() const noexcept { return m_parameters.empty(); }

	std::size_t GetParameterCount() const noexcept { return m_parameters.size(); }

	const std::vector<PassParameterDesc>& GetParameters() const noexcept { return m_parameters; }
	void SetAllVisibility(ShaderStageVisibility visibility) noexcept;

	const PassParameterDesc* FindParameter(std::string_view name) const noexcept;
	bool Matches(const PassParameterLayout& other) const noexcept;

	bool HasParameter(std::string_view name) const noexcept { return FindParameter(name) != nullptr; }

	std::uint32_t AddParameter(PassParameterDesc parameter);

	template <typename T>
	std::uint32_t Add(const char* name, ShaderStageVisibility visibility = ShaderStageVisibility::All, std::uint32_t arrayCount = 1)
	{
		static_assert(IsShaderParameterSemanticV<T>, "Add<T> requires a shader-parameter semantic type.");

		PassParameterDesc parameter{};
		parameter.Name = name != nullptr ? name : "";
		parameter.Kind = ShaderParameterSemanticTraits<T>::Kind;
		parameter.ResourceDomain = ShaderParameterSemanticTraits<T>::ResourceDomain;
		parameter.Access = ShaderParameterSemanticTraits<T>::Access;
		parameter.Visibility = visibility;
		parameter.ArrayCount = arrayCount;
		parameter.ValueSizeInBytes = PassParameterValueSize<T>::Value;
		return AddParameter(std::move(parameter));
	}

private:
	std::string m_debugName;
	std::vector<PassParameterDesc> m_parameters;
};
