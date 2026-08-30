#pragma once

#include "Cli/ICommand.h"

#include <span>
#include <string_view>

class ListTargetsCommand final : public ICommand
{
public:
	int Run(std::span<const std::string_view> args) const override;
};
