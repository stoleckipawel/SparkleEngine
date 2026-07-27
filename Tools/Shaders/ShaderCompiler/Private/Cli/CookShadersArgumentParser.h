#pragma once

#include "Backend/ShaderTarget.h"
#include "Cooking/ShaderCookSettings.h"

#include <cstddef>
#include <iosfwd>
#include <span>
#include <string>
#include <string_view>

class CookShadersArgumentParser final
{
  public:
	static bool Parse(
	    std::span<const std::string_view> arguments,
	    ShaderPackageCookSettings& outSettings,
	    std::string& outErrorMessage);
	static void PrintHelp(std::ostream& output);

  private:
	CookShadersArgumentParser(
	    std::span<const std::string_view> arguments,
	    ShaderPackageCookSettings& settings,
	    std::string& errorMessage) noexcept;

	bool ParseAll();
	bool ParseArgument(std::string_view argument);
	bool ParseFlag(std::string_view argument);
	bool ParseValueOption(std::string_view argument);
	bool ApplyValue(std::string_view argument, std::string_view value);
	bool AddTarget(std::string_view value);
	bool SetParallelCompileCount(std::string_view value);
	bool SetBooleanOption(std::string_view argument, std::string_view value);
	bool TakeValue(std::string_view argument, std::string_view& outValue);
	bool ValidateSelection();
	void SetUnknownArgumentError(std::string_view argument);

	static bool ContainsTarget(
	    std::span<const ShaderTarget> targets,
	    ShaderTarget target) noexcept;
	static bool TryParseTarget(
	    std::string_view value,
	    ShaderTarget& outTarget) noexcept;
	static bool TryParseBoolean(
	    std::string_view value,
	    bool& outValue) noexcept;

	std::span<const std::string_view> m_arguments;
	ShaderPackageCookSettings& m_settings;
	std::string& m_errorMessage;
	std::size_t m_index = 0;
	bool m_targetWasSpecified = false;
};
