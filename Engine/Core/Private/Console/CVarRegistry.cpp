#include "PCH.h"

#include "Core/Public/Console/CVar.h"

#include <string>

static const auto g_cvarRegistryLogger = Logging::GetOrCreateLogger("Core.Console");

ConsoleVariableBase::ConsoleVariableBase(std::string_view name, std::string_view description, std::type_index valueType) noexcept :
    m_name(name),
    m_description(description),
    m_valueType(valueType)
{
}

ConsoleVariableRegistry& ConsoleVariableRegistry::Get() noexcept
{
	static ConsoleVariableRegistry registry;
	return registry;
}

void ConsoleVariableRegistry::Register(ConsoleVariableBase& variable) noexcept
{
	const auto [iterator, inserted] = m_variablesByName.emplace(variable.GetName(), &variable);
	if (!inserted)
	{
		Diagnostics::Fatal(
		    g_cvarRegistryLogger,
		    __FILE__,
		    __LINE__,
		    "ConsoleVariableRegistry: Duplicate cvar registration for '" + std::string(variable.GetName()) + "'");
		return;
	}

	(void) iterator;
	m_variables.push_back(&variable);
}

ConsoleVariableBase* ConsoleVariableRegistry::Find(std::string_view name) noexcept
{
	const auto iterator = m_variablesByName.find(name);
	return iterator != m_variablesByName.end() ? iterator->second : nullptr;
}

const ConsoleVariableBase* ConsoleVariableRegistry::Find(std::string_view name) const noexcept
{
	const auto iterator = m_variablesByName.find(name);
	return iterator != m_variablesByName.end() ? iterator->second : nullptr;
}
