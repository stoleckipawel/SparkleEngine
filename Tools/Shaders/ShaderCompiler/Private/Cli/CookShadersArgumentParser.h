#pragma once

#include "RHI/Public/Shaders/ShaderTarget.h"
#include "Cooking/ShaderCookSettings.h"

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

class CookShadersArgumentParser final
{
public:
	static ShaderCookSettings Parse(std::span<const std::string_view> arguments);
	static void PrintHelp(std::ostream& output);

private:
	explicit CookShadersArgumentParser(std::span<const std::string_view> arguments) noexcept;

	ShaderCookSettings ParseAll();
	void ParseArgument(std::string_view argument);
	bool ConsumeFlag(std::string_view argument);
	bool ConsumeValueOption(std::string_view argument);
	void ApplyValue(std::string_view argument, std::string_view value);
	void AddTarget(std::string_view value);
	void SetParallelCompileCount(std::string_view value);
	void SetBooleanOption(std::string_view argument, std::string_view value);
	std::string_view TakeValue(std::string_view argument);
	void ValidateSelection() const;

	static bool ContainsTarget(std::span<const ShaderTarget> targets, ShaderTarget target) noexcept;
	static std::optional<ShaderTarget> ParseTarget(std::string_view value) noexcept;
	static std::optional<bool> ParseBoolean(std::string_view value) noexcept;

	std::span<const std::string_view> m_arguments;
	ShaderCookSettings m_settings;
	std::size_t m_index = 0;
	bool m_targetWasSpecified = false;
};
