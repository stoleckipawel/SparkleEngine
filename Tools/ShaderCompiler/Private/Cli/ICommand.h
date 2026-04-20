#pragma once

class ICommand
{
  public:
	virtual ~ICommand() = default;
	virtual int Run() const = 0;
};
