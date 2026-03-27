#pragma once

#include "ShaderParameterSemantics.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
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

constexpr ShaderStageVisibility operator|(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept
{
	return static_cast<ShaderStageVisibility>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

constexpr ShaderStageVisibility operator&(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept
{
	return static_cast<ShaderStageVisibility>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

constexpr ShaderStageVisibility& operator|=(ShaderStageVisibility& lhs, ShaderStageVisibility rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr bool HasAnyShaderStageVisibility(ShaderStageVisibility value, ShaderStageVisibility flags) noexcept
{
	return static_cast<std::uint8_t>(value & flags) != 0;
}

struct PassParameterDesc
{
	std::string Name;
	ShaderParameterSemanticKind Kind = ShaderParameterSemanticKind::ReadTexture;
	ShaderParameterResourceDomain ResourceDomain = ShaderParameterResourceDomain::None;
	ShaderParameterAccess Access = ShaderParameterAccess::None;
	ShaderStageVisibility Visibility = ShaderStageVisibility::All;
	std::uint32_t ArrayCount = 1;
	std::uint32_t ValueSizeInBytes = 0;

	bool IsArray() const noexcept
	{
		return ArrayCount > 1;
	}

	bool IsUniformData() const noexcept
	{
		return Kind == ShaderParameterSemanticKind::UniformData;
	}
};

template <typename T>
struct PassParameterValueSize
{
	static constexpr std::uint32_t Value = 0;
};

template <typename T>
struct PassParameterValueSize<UniformData<T>>
{
	static constexpr std::uint32_t Value = static_cast<std::uint32_t>(sizeof(T));
};

class PassParameterLayout final
{
  public:
	PassParameterLayout() = default;
	explicit PassParameterLayout(const char* debugName) : m_debugName(debugName != nullptr ? debugName : "") {}

	const std::string& GetDebugName() const noexcept
	{
		return m_debugName;
	}

	void SetDebugName(const char* debugName)
	{
		m_debugName = debugName != nullptr ? debugName : "";
	}

	bool IsEmpty() const noexcept
	{
		return m_parameters.empty();
	}

	std::size_t GetParameterCount() const noexcept
	{
		return m_parameters.size();
	}

	const std::vector<PassParameterDesc>& GetParameters() const noexcept
	{
		return m_parameters;
	}

	const PassParameterDesc* FindParameter(const char* name) const noexcept
	{
		if (name == nullptr)
		{
			return nullptr;
		}

		auto it = std::find_if(
		    m_parameters.begin(),
		    m_parameters.end(),
		    [name](const PassParameterDesc& parameter)
		    {
			    return parameter.Name == name;
		    });

		return it != m_parameters.end() ? &(*it) : nullptr;
	}

	bool HasParameter(const char* name) const noexcept
	{
		return FindParameter(name) != nullptr;
	}

	std::uint32_t AddParameter(PassParameterDesc parameter)
	{
		assert(!parameter.Name.empty());
		assert(parameter.ArrayCount > 0);
		assert(!HasParameter(parameter.Name.c_str()));

		m_parameters.push_back(std::move(parameter));
		return static_cast<std::uint32_t>(m_parameters.size() - 1);
	}

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