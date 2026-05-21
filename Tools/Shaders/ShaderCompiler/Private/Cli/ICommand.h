#pragma once

#include <span>
#include <string_view>

class ICommand
{
  public:
	virtual ~ICommand() = default;
	virtual int Run(std::span<const std::string_view> args) const = 0;
};
