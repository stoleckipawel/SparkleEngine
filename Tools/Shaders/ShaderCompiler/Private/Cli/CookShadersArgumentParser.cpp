#include "PCH.h"

#include "Cli/CookShadersArgumentParser.h"

#include "Constants/ShaderCompilerConstants.h"
#include "Core/Public/Strings/StringUtils.h"

#include <charconv>
#include <filesystem>
#include <ostream>
#include <system_error>

bool CookShadersArgumentParser::Parse(
    std::span<const std::string_view> arguments,
    ShaderPackageCookSettings& outSettings,
    std::string& outErrorMessage)
{
	CookShadersArgumentParser parser(
	    arguments,
	    outSettings,
	    outErrorMessage);
	return parser.ParseAll();
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

CookShadersArgumentParser::CookShadersArgumentParser(
    std::span<const std::string_view> arguments,
    ShaderPackageCookSettings& settings,
    std::string& errorMessage) noexcept :
    m_arguments(arguments),
    m_settings(settings),
    m_errorMessage(errorMessage)
{
}

bool CookShadersArgumentParser::ParseAll()
{
	for (m_index = 0; m_index < m_arguments.size(); ++m_index)
	{
		if (!ParseArgument(m_arguments[m_index]))
		{
			return false;
		}
	}

	return ValidateSelection();
}

bool CookShadersArgumentParser::ParseArgument(std::string_view argument)
{
	if (ParseFlag(argument))
	{
		return true;
	}

	if (ParseValueOption(argument))
	{
		return true;
	}

	if (!m_errorMessage.empty())
	{
		return false;
	}

	SetUnknownArgumentError(argument);
	return false;
}

bool CookShadersArgumentParser::ParseFlag(std::string_view argument)
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

bool CookShadersArgumentParser::ParseValueOption(std::string_view argument)
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

	std::string_view value;
	if (!TakeValue(argument, value))
	{
		return false;
	}

	return ApplyValue(argument, value);
}

bool CookShadersArgumentParser::ApplyValue(
    std::string_view argument,
    std::string_view value)
{
	if (argument == "--cache-dir")
	{
		m_settings.cacheDirectory = std::filesystem::path(std::string(value));
		return true;
	}

	if (argument == "--package")
	{
		m_settings.packageId = value;
		return true;
	}

	if (argument == "--shader-id")
	{
		m_settings.shaderId = value;
		return true;
	}

	if (argument == "--target")
	{
		return AddTarget(value);
	}

	if (argument == "--backend")
	{
		m_settings.backendName = value;
		return true;
	}

	if (argument == "--debug-artifacts")
	{
		m_settings.debugArtifactDirectory = std::filesystem::path(std::string(value));
		return true;
	}

	if (argument == "--analysis")
	{
		for (const std::string_view token : Strings::Split(value, ',', false))
		{
			m_settings.analysisPasses.emplace_back(token);
		}
		return true;
	}

	if (argument == "--parallel-compiles")
	{
		return SetParallelCompileCount(value);
	}

	return SetBooleanOption(argument, value);
}

bool CookShadersArgumentParser::AddTarget(std::string_view value)
{
	ShaderTarget target = kDefaultShaderTarget;
	if (!TryParseTarget(value, target))
	{
		m_errorMessage = "Unknown shader target '" + std::string(value) + "'";
		return false;
	}

	if (!m_targetWasSpecified)
	{
		m_settings.targets.clear();
		m_targetWasSpecified = true;
	}

	if (!ContainsTarget(m_settings.targets, target))
	{
		m_settings.targets.push_back(target);
	}

	return true;
}

bool CookShadersArgumentParser::SetParallelCompileCount(std::string_view value)
{
	std::uint32_t count = 0;
	const auto parsed =
	    std::from_chars(value.data(), value.data() + value.size(), count);
	if (parsed.ec != std::errc{} ||
	    parsed.ptr != value.data() + value.size() ||
	    count < 1 ||
	    count > 8)
	{
		m_errorMessage =
		    "Expected a value from 1 through 8 after --parallel-compiles";
		return false;
	}

	m_settings.maximumParallelCompiles = count;
	return true;
}

bool CookShadersArgumentParser::SetBooleanOption(
    std::string_view argument,
    std::string_view value)
{
	bool parsedValue = false;
	if (!TryParseBoolean(value, parsedValue))
	{
		m_errorMessage =
		    "Expected on|off after " + std::string(argument);
		return false;
	}

	if (argument == "--warnings-as-errors")
	{
		m_settings.treatWarningsAsErrors = parsedValue;
	}
	else
	{
		m_settings.stripDebugInfo = parsedValue;
	}

	return true;
}

bool CookShadersArgumentParser::TakeValue(
    std::string_view argument,
    std::string_view& outValue)
{
	if (m_index + 1 >= m_arguments.size())
	{
		m_errorMessage = "Missing value after " + std::string(argument);
		return false;
	}

	outValue = m_arguments[++m_index];
	return true;
}

bool CookShadersArgumentParser::ValidateSelection()
{
	if (!m_settings.packageId.empty() && !m_settings.shaderId.empty())
	{
		m_errorMessage = "Use either --package or --shader-id, not both";
		return false;
	}

	m_errorMessage.clear();
	return true;
}

void CookShadersArgumentParser::SetUnknownArgumentError(
    std::string_view argument)
{
	m_errorMessage = "Unknown cook argument '" + std::string(argument) + "'";
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

bool CookShadersArgumentParser::TryParseTarget(
    std::string_view value,
    ShaderTarget& outTarget) noexcept
{
	for (std::uint16_t candidate = static_cast<std::uint16_t>(ShaderTarget::DxilSm60);
	     candidate <= static_cast<std::uint16_t>(ShaderTarget::SpirV16);
	     ++candidate)
	{
		const auto target = static_cast<ShaderTarget>(candidate);
		if (value == GetShaderTargetName(target))
		{
			outTarget = target;
			return true;
		}
	}

	return false;
}

bool CookShadersArgumentParser::TryParseBoolean(
    std::string_view value,
    bool& outValue) noexcept
{
	if (value == "on" || value == "true")
	{
		outValue = true;
		return true;
	}

	if (value == "off" || value == "false")
	{
		outValue = false;
		return true;
	}

	return false;
}
