#include "PCH.h"

#include "Cli/CookShadersArgumentParser.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"

#include <charconv>
#include <filesystem>
#include <ostream>
#include <system_error>

ShaderPackageCookSettings CookShadersArgumentParser::Parse(std::span<const std::string_view> arguments)
{
	return CookShadersArgumentParser(arguments).ParseAll();
}

void CookShadersArgumentParser::PrintHelp(std::ostream& output)
{
	output << "Usage:\n"
	       << "  ShaderCompiler cook [--package <package-id> | --shader-id <registered-shader-name>] [options]\n\n"
	       << "Cooks typed shader registrations into .sparkshader packages and ShaderPackageRegistry.sreg.\n\n"
	       << "Selection options:\n"
	       << "  --package <package-id>      Cook one registered shader package.\n"
	       << "  --shader-id <shader-id>     Cook one registered shader entry by shader id.\n\n"
	       << "Cook options:\n"
	       << "  --no-cache                  Disable the local compile cache.\n"
	       << "  --cache-dir <path>          Override the local compile cache directory.\n"
	       << "  --target <name>             Add a codegen target such as DxilSm66 or SpirV16. "
	          "Defaults to DxilSm66 and SpirV16.\n"
	       << "  --backend <name>            Select a compiler backend, or auto.\n"
	       << "  --debug-artifacts <dir>     Write debug artifact bundles outside runtime packages.\n"
	       << "  --analysis <pass[,pass]>    Run optional analysis report passes such as cooked-shader-stats.\n"
	       << "  --parallel-compiles <1-8>   Bound concurrent compiler sessions. Defaults to 4; use 1 as the serial oracle.\n"
	       << "  --debug-info                Request backend debug information and symbol emission where supported.\n"
	       << "  --disable-optimizations     Build shaders without backend optimization passes.\n"
	       << "  --warnings-as-errors <on|off>  Treat shader warnings as hard errors. Defaults to on.\n"
	       << "  --strip-debug <on|off>      Strip embedded debug info from final runtime binaries when supported.\n";
}

CookShadersArgumentParser::CookShadersArgumentParser(std::span<const std::string_view> arguments) noexcept :
    m_arguments(arguments)
{
}

ShaderPackageCookSettings CookShadersArgumentParser::ParseAll()
{
	for (m_index = 0; m_index < m_arguments.size(); ++m_index)
	{
		ParseArgument(m_arguments[m_index]);
	}

	ValidateSelection();
	return std::move(m_settings);
}

void CookShadersArgumentParser::ParseArgument(std::string_view argument)
{
	if (ConsumeFlag(argument))
	{
		return;
	}

	if (ConsumeValueOption(argument))
	{
		return;
	}

	throw Diagnostics::Error("Unknown cook argument '" + std::string(argument) + "'");
}

bool CookShadersArgumentParser::ConsumeFlag(std::string_view argument)
{
	if (argument == "--no-cache")
	{
		m_settings.useCache = false;
		return true;
	}

	if (argument == "--debug-info")
	{
		m_settings.enableDebugInfo = true;
		return true;
	}

	if (argument == "--disable-optimizations")
	{
		m_settings.enableOptimizations = false;
		return true;
	}

	return false;
}

bool CookShadersArgumentParser::ConsumeValueOption(std::string_view argument)
{
	if (argument != "--cache-dir" &&
	    argument != "--package" &&
	    argument != "--shader-id" &&
	    argument != "--target" &&
	    argument != "--backend" &&
	    argument != "--debug-artifacts" &&
	    argument != "--analysis" &&
	    argument != "--parallel-compiles" &&
	    argument != "--warnings-as-errors" &&
	    argument != "--strip-debug")
	{
		return false;
	}

	ApplyValue(argument, TakeValue(argument));
	return true;
}

void CookShadersArgumentParser::ApplyValue(
    std::string_view argument,
    std::string_view value)
{
	if (argument == "--cache-dir")
	{
		m_settings.cacheDirectory = std::filesystem::path(std::string(value));
		return;
	}

	if (argument == "--package")
	{
		m_settings.packageId = value;
		return;
	}

	if (argument == "--shader-id")
	{
		m_settings.shaderId = value;
		return;
	}

	if (argument == "--target")
	{
		AddTarget(value);
		return;
	}

	if (argument == "--backend")
	{
		m_settings.backendName = value;
		return;
	}

	if (argument == "--debug-artifacts")
	{
		m_settings.debugArtifactDirectory = std::filesystem::path(std::string(value));
		return;
	}

	if (argument == "--analysis")
	{
		for (const std::string_view token : Strings::Split(value, ',', false))
		{
			m_settings.analysisPasses.emplace_back(token);
		}
		return;
	}

	if (argument == "--parallel-compiles")
	{
		SetParallelCompileCount(value);
		return;
	}

	SetBooleanOption(argument, value);
}

void CookShadersArgumentParser::AddTarget(std::string_view value)
{
	const std::optional<ShaderTarget> target = ParseTarget(value);
	if (!target)
	{
		throw Diagnostics::Error("Unknown shader target '" + std::string(value) + "'");
	}

	if (!m_targetWasSpecified)
	{
		m_settings.targets.clear();
		m_targetWasSpecified = true;
	}

	if (!ContainsTarget(m_settings.targets, *target))
	{
		m_settings.targets.push_back(*target);
	}
}

void CookShadersArgumentParser::SetParallelCompileCount(std::string_view value)
{
	std::uint32_t count = 0;
	const auto parsed =
	    std::from_chars(value.data(), value.data() + value.size(), count);
	if (parsed.ec != std::errc{} ||
	    parsed.ptr != value.data() + value.size() ||
	    count < 1 ||
	    count > 8)
	{
		throw Diagnostics::Error("Expected a value from 1 through 8 after --parallel-compiles");
	}

	m_settings.maximumParallelCompiles = count;
}

void CookShadersArgumentParser::SetBooleanOption(
    std::string_view argument,
    std::string_view value)
{
	const std::optional<bool> parsedValue = ParseBoolean(value);
	if (!parsedValue)
	{
		throw Diagnostics::Error("Expected on|off after " + std::string(argument));
	}

	if (argument == "--warnings-as-errors")
	{
		m_settings.treatWarningsAsErrors = *parsedValue;
	}
	else
	{
		m_settings.stripDebugInfo = *parsedValue;
	}
}

std::string_view CookShadersArgumentParser::TakeValue(std::string_view argument)
{
	if (m_index + 1 >= m_arguments.size())
	{
		throw Diagnostics::Error("Missing value after " + std::string(argument));
	}

	return m_arguments[++m_index];
}

void CookShadersArgumentParser::ValidateSelection() const
{
	if (!m_settings.packageId.empty() && !m_settings.shaderId.empty())
	{
		throw Diagnostics::Error("Use either --package or --shader-id, not both");
	}
}

bool CookShadersArgumentParser::ContainsTarget(
    std::span<const ShaderTarget> targets,
    ShaderTarget target) noexcept
{
	for (const ShaderTarget existingTarget : targets)
	{
		if (existingTarget == target)
		{
			return true;
		}
	}

	return false;
}

std::optional<ShaderTarget> CookShadersArgumentParser::ParseTarget(std::string_view value) noexcept
{
	for (std::uint16_t candidate = static_cast<std::uint16_t>(ShaderTarget::DxilSm60);
	     candidate <= static_cast<std::uint16_t>(ShaderTarget::SpirV16);
	     ++candidate)
	{
		const auto target = static_cast<ShaderTarget>(candidate);
		if (value == GetShaderTargetName(target))
		{
			return target;
		}
	}

	return std::nullopt;
}

std::optional<bool> CookShadersArgumentParser::ParseBoolean(std::string_view value) noexcept
{
	if (value == "on" || value == "true")
	{
		return true;
	}

	if (value == "off" || value == "false")
	{
		return false;
	}

	return std::nullopt;
}
