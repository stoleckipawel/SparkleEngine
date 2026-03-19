#pragma once

#include "Core/Public/CoreAPI.h"

#include <unordered_map>
#include <string_view>
#include <vector>

class ConsoleVariableBase;

class SPARKLE_CORE_API ConsoleVariableRegistry final
{
  public:
	static ConsoleVariableRegistry& Get() noexcept;

	void Register(ConsoleVariableBase& variable) noexcept;

	ConsoleVariableBase* Find(std::string_view name) noexcept;
	const ConsoleVariableBase* Find(std::string_view name) const noexcept;

	const std::vector<ConsoleVariableBase*>& GetVariables() const noexcept { return m_variables; }

  private:
	std::unordered_map<std::string_view, ConsoleVariableBase*> m_variablesByName;
	std::vector<ConsoleVariableBase*> m_variables;
};