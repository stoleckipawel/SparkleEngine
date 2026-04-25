#pragma once

#include "Cli/ICommand.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"

#include <span>
#include <string_view>

class InspectPackageCommand final : public ICommand
{
  public:
	int Run(std::span<const std::string_view> args) const override;
};