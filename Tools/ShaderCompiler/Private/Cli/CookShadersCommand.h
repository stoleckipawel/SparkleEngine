#pragma once

#include "Cli/ICommand.h"
#include "Cooking/ShaderPackageCooker.h"

#include <span>
#include <string>
#include <string_view>

class CookShadersCommand final : public ICommand
{
  public:
	int Run(std::span<const std::string_view> args) const override;

  private:
	static bool TryParseArguments(
	    std::span<const std::string_view> args,
	    ShaderPackageCookSettings& outSettings,
	    std::string& outErrorMessage);
};
