#include "LauncherShellArguments.h"

#include "SparkleLauncher/BuildProfileCatalog.h"

#include <optional>
#include <ostream>
#include <string_view>

namespace SparkleLauncher
{
	struct LauncherCleanScopeOption final
	{
		std::string_view Name;
		CleanScope Scope;
	};

	class LauncherShellArgumentParser final
	{
	public:
		LauncherShellArgumentParser(int argc, char** argv, LauncherShellArguments& outArguments, std::ostream& error) noexcept;

		bool Parse();

	private:
		bool ParseCurrentArgument(std::string_view argument);
		bool ParseProfile(BuildProfileTarget target, std::string& outProfile);
		bool ParseWorkspaceIde();
		bool ParseWorkspaceCompiler();
		bool ParseCleanScope();
		std::optional<std::string_view> ReadRequiredValue(std::string_view missingValueMessage);

		static bool IsProfileTarget(std::string_view profileName, BuildProfileTarget target);
		static bool TryParseCleanScope(std::string_view text, CleanScope& outScope) noexcept;

		int m_argumentCount = 0;
		char** m_arguments = nullptr;
		int m_index = 1;
		LauncherShellArguments* m_outArguments = nullptr;
		std::ostream* m_error = nullptr;
	};

	LauncherShellArgumentParser::LauncherShellArgumentParser(
	    int argc,
	    char** argv,
	    LauncherShellArguments& outArguments,
	    std::ostream& error) noexcept :
	    m_argumentCount(argc),
	    m_arguments(argv),
	    m_outArguments(&outArguments),
	    m_error(&error)
	{
	}

	bool LauncherShellArgumentParser::Parse()
	{
		for (; m_index < m_argumentCount; ++m_index)
		{
			const std::string_view argument(m_arguments[m_index]);
			if (!ParseCurrentArgument(argument))
			{
				return false;
			}
		}

		return true;
	}

	bool LauncherShellArgumentParser::ParseCurrentArgument(std::string_view argument)
	{
		if (argument == "--help" || argument == "-h" || argument == "/?")
		{
			m_outArguments->ShowHelp = true;
			return true;
		}

		if (argument == "--root")
		{
			const std::optional<std::string_view> value = ReadRequiredValue("SparkleLauncher: --root requires a path.\n");
			if (value.has_value())
			{
				m_outArguments->StartPath = *value;
			}
			return value.has_value();
		}

		if (argument == "--editor-profile")
		{
			return ParseProfile(BuildProfileTarget::Editor, m_outArguments->EditorProfile);
		}

		if (argument == "--runtime-profile")
		{
			return ParseProfile(BuildProfileTarget::Game, m_outArguments->RuntimeProfile);
		}

		if (argument == "--dry-run")
		{
			if (m_index + 1 < m_argumentCount && m_arguments[m_index + 1][0] != '-')
			{
				m_outArguments->DryRunOperationId = m_arguments[++m_index];
			}
			else
			{
				m_outArguments->DryRunOperationId = "workspace.sync-code";
			}
			return true;
		}

		if (argument == "--run")
		{
			if (m_index + 1 >= m_argumentCount || m_arguments[m_index + 1][0] == '-')
			{
				*m_error << "SparkleLauncher: --run requires an operation id.\n";
				return false;
			}
			m_outArguments->RunOperationId = m_arguments[++m_index];
			return true;
		}

		if (argument == "--ide")
		{
			return ParseWorkspaceIde();
		}

		if (argument == "--compiler")
		{
			return ParseWorkspaceCompiler();
		}

		if (argument == "--force-recook")
		{
			m_outArguments->RequestedCookMode = CookMode::Force;
			return true;
		}

		if (argument == "--confirm-force-recook")
		{
			m_outArguments->ForceRecookConfirmed = true;
			return true;
		}

		if (argument == "--clean-scope")
		{
			return ParseCleanScope();
		}

		if (argument == "--confirm-clean")
		{
			m_outArguments->CleanConfirmed = true;
			return true;
		}

		*m_error << "SparkleLauncher: unexpected argument '" << argument << "'.\n";
		return false;
	}

	bool LauncherShellArgumentParser::ParseProfile(BuildProfileTarget target, std::string& outProfile)
	{
		const std::string_view option = target == BuildProfileTarget::Editor ? "--editor-profile" : "--runtime-profile";
		const std::string missingValueMessage = "SparkleLauncher: " + std::string(option) + " requires a profile.\n";
		const std::optional<std::string_view> profile = ReadRequiredValue(missingValueMessage);
		if (!profile.has_value())
		{
			return false;
		}

		if (!IsProfileTarget(*profile, target))
		{
			*m_error << "SparkleLauncher: unsupported " << (target == BuildProfileTarget::Editor ? "editor" : "runtime") << " profile '"
			         << *profile << "'.\n";
			return false;
		}

		outProfile = *profile;
		return true;
	}

	bool LauncherShellArgumentParser::ParseWorkspaceIde()
	{
		const std::optional<std::string_view> value = ReadRequiredValue("SparkleLauncher: --ide requires visual-studio or rider.\n");
		if (!value.has_value())
		{
			return false;
		}

		WorkspaceIde ide = WorkspaceIde::VisualStudio;
		if (!TryParseWorkspaceIde(*value, ide))
		{
			*m_error << "SparkleLauncher: unsupported IDE '" << *value << "'.\n";
			return false;
		}

		m_outArguments->WorkspaceIdePreference = ide;
		return true;
	}

	bool LauncherShellArgumentParser::ParseWorkspaceCompiler()
	{
		const std::optional<std::string_view> value = ReadRequiredValue("SparkleLauncher: --compiler requires msvc or clang-cl.\n");
		if (!value.has_value())
		{
			return false;
		}

		WorkspaceCompiler compiler = WorkspaceCompiler::Msvc;
		if (!TryParseWorkspaceCompiler(*value, compiler))
		{
			*m_error << "SparkleLauncher: unsupported compiler '" << *value << "'.\n";
			return false;
		}

		m_outArguments->WorkspaceCompilerPreference = compiler;
		return true;
	}

	bool LauncherShellArgumentParser::ParseCleanScope()
	{
		const std::optional<std::string_view> value = ReadRequiredValue("SparkleLauncher: --clean-scope requires a scope.\n");
		if (!value.has_value())
		{
			return false;
		}

		CleanScope scope = CleanScope::CookedOutputs;
		if (!TryParseCleanScope(*value, scope))
		{
			*m_error << "SparkleLauncher: unsupported clean scope '" << *value << "'.\n";
			return false;
		}

		m_outArguments->RequestedCleanScope = scope;
		return true;
	}

	std::optional<std::string_view> LauncherShellArgumentParser::ReadRequiredValue(std::string_view missingValueMessage)
	{
		if (m_index + 1 >= m_argumentCount)
		{
			*m_error << missingValueMessage;
			return std::nullopt;
		}

		return std::string_view(m_arguments[++m_index]);
	}

	bool LauncherShellArgumentParser::IsProfileTarget(std::string_view profileName, BuildProfileTarget target)
	{
		const std::optional<BuildProfile> profile = FindBuildProfile(profileName);
		return profile.has_value() && profile->Target == target;
	}

	bool LauncherShellArgumentParser::TryParseCleanScope(std::string_view text, CleanScope& outScope) noexcept
	{
		static constexpr LauncherCleanScopeOption options[] = {
		    {"cooked", CleanScope::CookedOutputs},
		    {"build-tree", CleanScope::BuildTree},
		    {"artifacts", CleanScope::ArtifactOutputs},
		    {"workspace-state", CleanScope::WorkspaceState},
		    {"shader-cache", CleanScope::ShaderCache},
		    {"deps", CleanScope::ThirdPartyDependencyCache},
		    {"logs", CleanScope::Logs},
		    {"clean-all", CleanScope::PristineGeneratedWorkspace}};

		for (const LauncherCleanScopeOption& option : options)
		{
			if (option.Name == text)
			{
				outScope = option.Scope;
				return true;
			}
		}

		return false;
	}

	bool ParseLauncherShellArguments(int argc, char** argv, LauncherShellArguments& outArguments, std::ostream& error)
	{
		LauncherShellArgumentParser parser(argc, argv, outArguments, error);
		return parser.Parse();
	}

	void PrintLauncherShellUsage(std::ostream& output)
	{
		output << "Usage:\n"
		       << "  SparkleLauncher [--root <repo-root>] [--editor-profile <profile>] [--runtime-profile <profile>] "
		          "[--ide <visual-studio|rider>] [--compiler <msvc|clang-cl>] "
		          "[--clean-scope <scope>] "
		          "[--confirm-clean] [--force-recook] [--confirm-force-recook] [--dry-run [operation-id]] [--run <operation-id>]\n"
		       << "\n"
		       << "Examples:\n"
		       << "  SparkleLauncher --dry-run\n"
		       << "  SparkleLauncher --runtime-profile DevelopmentGame --dry-run cook.shaders\n"
		       << "  SparkleLauncher --force-recook --dry-run cook.all\n"
		       << "  SparkleLauncher --clean-scope cooked --dry-run workspace.clean\n"
		       << "  SparkleLauncher --clean-scope clean-all --dry-run workspace.clean\n";
	}
}
