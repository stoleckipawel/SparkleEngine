#pragma once

#include "Cli/ICommand.h"

class CookShadersCommand final : public ICommand
{
  public:
	int Run() const override;
};
