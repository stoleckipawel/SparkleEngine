#pragma once

#include "Cli/ICommand.h"

class InspectManifestCommand final : public ICommand
{
  public:
	int Run() const override;
};
