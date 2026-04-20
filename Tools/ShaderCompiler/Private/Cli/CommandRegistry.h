#pragma once

#include "Cli/ICommand.h"

#include <memory>
#include <ostream>
#include <string_view>
#include <vector>

class CommandRegistry final
{
  public:
	CommandRegistry();
	const ICommand* Find(std::string_view verb) const noexcept;
	void PrintUsage(std::ostream& output) const;

  private:
	struct Registration final
	{
		std::vector<std::string_view> verbs;
		std::shared_ptr<ICommand> command;
		std::string_view usageLine;
		std::string_view legacyUsageLine;
	};

	std::vector<Registration> m_registrations;
};
