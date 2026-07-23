#include "PCH.h"

#include "ShaderParameters/PassParameterLayout.h"

#include <algorithm>
#include <cassert>
#include <utility>

ShaderStageVisibility operator|(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept
{
	return static_cast<ShaderStageVisibility>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}

ShaderStageVisibility operator&(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept
{
	return static_cast<ShaderStageVisibility>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
}

ShaderStageVisibility& operator|=(ShaderStageVisibility& lhs, ShaderStageVisibility rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

bool HasAnyShaderStageVisibility(ShaderStageVisibility value, ShaderStageVisibility flags) noexcept
{
	return static_cast<std::uint8_t>(value & flags) != 0;
}

PassParameterLayout::PassParameterLayout() = default;

PassParameterLayout::PassParameterLayout(const char* debugName)
    : m_debugName(debugName != nullptr ? debugName : "")
{
}

const PassParameterDesc* PassParameterLayout::FindParameter(const char* name) const noexcept
{
	if (name == nullptr)
	{
		return nullptr;
	}

	const auto it = std::find_if(
	    m_parameters.begin(),
	    m_parameters.end(),
	    [name](const PassParameterDesc& parameter)
	    {
		    return parameter.Name == name;
	    });

	return it != m_parameters.end() ? &(*it) : nullptr;
}

std::uint32_t PassParameterLayout::AddParameter(PassParameterDesc parameter)
{
	assert(!parameter.Name.empty());
	assert(parameter.ArrayCount > 0);
	assert(!HasParameter(parameter.Name.c_str()));

	m_parameters.push_back(std::move(parameter));
	return static_cast<std::uint32_t>(m_parameters.size() - 1);
}
