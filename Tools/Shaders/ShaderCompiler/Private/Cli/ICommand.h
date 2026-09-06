#pragma once

#include <span>
#include <string_view>

class ICommand
{
public:
	virtual ~ICommand() = default;
	ICommand(const ICommand&) = delete;
	ICommand& operator=(const ICommand&) = delete;
	ICommand(ICommand&&) = delete;
	ICommand& operator=(ICommand&&) = delete;

	virtual int Run(std::span<const std::string_view> args) const = 0;

protected:
	ICommand() = default;
};
