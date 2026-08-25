#include "PCH.h"

#include "ShaderParameters/PassParameterLayout.h"

#include <algorithm>
#include <cassert>
#include <utility>

ShaderStageVisibility operator|(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept
{
	return static_cast<ShaderStageVisibility>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}

ShaderStageVisibility operator&(ShaderStageVisibility lhs, ShaderStageVisibility rhs) noexcept
{
	return static_cast<ShaderStageVisibility>(static_cast<std::uint16_t>(lhs) & static_cast<std::uint16_t>(rhs));
}

ShaderStageVisibility& operator|=(ShaderStageVisibility& lhs, ShaderStageVisibility rhs) noexcept
{
	lhs = lhs | rhs;
	return lhs;
}

bool HasAnyShaderStageVisibility(ShaderStageVisibility value, ShaderStageVisibility flags) noexcept
{
	return static_cast<std::uint16_t>(value & flags) != 0;
}

PassParameterLayout::PassParameterLayout() = default;

PassParameterLayout::PassParameterLayout(const char* debugName) :
    m_debugName(debugName != nullptr ? debugName : "")
{
}

const PassParameterDesc* PassParameterLayout::FindParameter(std::string_view name) const noexcept
{
	const auto it = std::find_if(
	    m_parameters.begin(),
	    m_parameters.end(),
	    [name](const PassParameterDesc& parameter) { return parameter.Name == name; });

	return it != m_parameters.end() ? &(*it) : nullptr;
}

bool PassParameterLayout::Matches(const PassParameterLayout& other) const noexcept
{
	if (m_parameters.size() != other.m_parameters.size())
	{
		return false;
	}

	for (const PassParameterDesc& lhs : m_parameters)
	{
		const PassParameterDesc* const rhs = other.FindParameter(lhs.Name);
		if (rhs == nullptr)
		{
			return false;
		}
		if (lhs.Kind != rhs->Kind || lhs.ResourceDomain != rhs->ResourceDomain || lhs.Access != rhs->Access
		    || lhs.Visibility != rhs->Visibility || lhs.ArrayCount != rhs->ArrayCount || lhs.ValueSizeInBytes != rhs->ValueSizeInBytes)
		{
			return false;
		}
	}

	return true;
}

void PassParameterLayout::SetAllVisibility(ShaderStageVisibility visibility) noexcept
{
	for (PassParameterDesc& parameter : m_parameters)
	{
		parameter.Visibility = visibility;
	}
}

std::uint32_t PassParameterLayout::AddParameter(PassParameterDesc parameter)
{
	assert(!parameter.Name.empty());
	assert(parameter.ArrayCount > 0);
	const auto existing =
	    std::ranges::find_if(m_parameters, [&parameter](const PassParameterDesc& candidate) { return candidate.Name == parameter.Name; });
	if (existing != m_parameters.end())
	{
		assert(existing->Kind == parameter.Kind);
		assert(existing->ResourceDomain == parameter.ResourceDomain);
		assert(existing->Access == parameter.Access);
		assert(existing->ArrayCount == parameter.ArrayCount);
		assert(existing->ValueSizeInBytes == parameter.ValueSizeInBytes);
		existing->Visibility |= parameter.Visibility;
		return static_cast<std::uint32_t>(std::distance(m_parameters.begin(), existing));
	}

	m_parameters.push_back(std::move(parameter));
	return static_cast<std::uint32_t>(m_parameters.size() - 1);
}
