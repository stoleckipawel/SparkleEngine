#include "VisualStudioToolchainDiscovery.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Core/Public/Process/ChildProcess.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	static std::optional<std::filesystem::path> ResolveProgramFilesX86()
	{
		std::string value;
		if (Environment::TryGetVariable("ProgramFiles(x86)", value))
		{
			return std::filesystem::path(value);
		}
		if (Environment::TryGetVariable("ProgramFiles", value))
		{
			return std::filesystem::path(value);
		}
		return std::nullopt;
	}

	static std::vector<std::filesystem::path> GetProgramFilesRoots()
	{
		std::vector<std::filesystem::path> roots;
		std::string value;
		if (Environment::TryGetVariable("ProgramFiles", value))
		{
			roots.emplace_back(value);
		}
		if (Environment::TryGetVariable("ProgramFiles(x86)", value))
		{
			roots.emplace_back(value);
		}
		return roots;
	}

	static std::optional<std::filesystem::path> ResolveVswherePath()
	{
		std::string overridePath;
		if (Environment::TryGetVariable("SPARKLE_VSWHERE_EXE", overridePath))
		{
			std::filesystem::path path(overridePath);
			std::error_code errorCode;
			if (std::filesystem::exists(path, errorCode))
			{
				return path;
			}
		}

		const std::optional<std::filesystem::path> programFiles = ResolveProgramFilesX86();
		if (!programFiles.has_value())
		{
			return std::nullopt;
		}

		const std::filesystem::path path = *programFiles / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
		std::error_code errorCode;
		return std::filesystem::exists(path, errorCode) ? std::optional<std::filesystem::path>(path) : std::nullopt;
	}

	static std::optional<std::filesystem::path> FindVisualStudioInstallWithCppTools()
	{
		const std::vector<std::string> versions = {"18", "2026", "17", "2022"};
		const std::vector<std::string> editions = {"Community", "Professional", "Enterprise", "BuildTools"};
		for (const std::filesystem::path& root : GetProgramFilesRoots())
		{
			const std::filesystem::path visualStudioRoot = root / "Microsoft Visual Studio";
			for (const std::string& version : versions)
			{
				for (const std::string& edition : editions)
				{
					const std::filesystem::path installRoot = visualStudioRoot / version / edition;
					const std::filesystem::path vcToolsRoot = installRoot / "VC" / "Tools" / "MSVC";
					const std::filesystem::path msbuild = installRoot / "MSBuild" / "Current" / "Bin" / "MSBuild.exe";
					std::error_code errorCode;
					if (std::filesystem::is_directory(vcToolsRoot, errorCode) && std::filesystem::is_regular_file(msbuild, errorCode))
					{
						return installRoot;
					}
				}
			}
		}
		return std::nullopt;
	}

	static std::string TrimCopy(std::string value)
	{
		while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
		{
			value.pop_back();
		}
		const auto first = std::find_if(value.begin(), value.end(), [](unsigned char character) { return !std::isspace(character); });
		value.erase(value.begin(), first);
		return value;
	}

	static std::optional<std::filesystem::path> QueryVisualStudioInstallWithCppTools(const std::filesystem::path& vswherePath)
	{
		if (vswherePath.empty())
		{
			return std::nullopt;
		}

		const Process::ChildProcessResult result = Process::ChildProcess::Run(
		    Process::ChildProcessRequest{
		        .ExecutablePath = vswherePath,
		        .Arguments =
		            {"-latest",
		                "-products",
		                "*",
		                "-requires",
		                "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
		                "-property",
		                "installationPath"},
		    });
		if (!result.Succeeded())
		{
			return std::nullopt;
		}

		const std::string path = TrimCopy(result.CapturedOutput);
		std::error_code errorCode;
		return !path.empty() && std::filesystem::is_directory(path, errorCode) ? std::optional<std::filesystem::path>(path) : std::nullopt;
	}

	static std::optional<std::filesystem::path> ResolveVisualStudioInstallerPath()
	{
		const std::optional<std::filesystem::path> programFiles = ResolveProgramFilesX86();
		if (!programFiles.has_value())
		{
			return std::nullopt;
		}

		const std::filesystem::path path = *programFiles / "Microsoft Visual Studio" / "Installer" / "setup.exe";
		std::error_code errorCode;
		return std::filesystem::is_regular_file(path, errorCode) ? std::optional<std::filesystem::path>(path) : std::nullopt;
	}

	static std::optional<std::filesystem::path> FindVisualStudioClangCl(const std::filesystem::path& installationPath)
	{
		if (installationPath.empty())
		{
			return std::nullopt;
		}

		const std::filesystem::path path = installationPath / "VC" / "Tools" / "Llvm" / "x64" / "bin" / "clang-cl.exe";
		std::error_code errorCode;
		return std::filesystem::is_regular_file(path, errorCode) ? std::optional<std::filesystem::path>(path) : std::nullopt;
	}

	static std::optional<std::filesystem::path> FindVisualStudioIde(const std::filesystem::path& installationPath)
	{
		if (installationPath.empty())
		{
			return std::nullopt;
		}

		const std::filesystem::path path = installationPath / "Common7" / "IDE" / "devenv.exe";
		std::error_code errorCode;
		return std::filesystem::is_regular_file(path, errorCode) ? std::optional<std::filesystem::path>(path) : std::nullopt;
	}

	static std::optional<std::string> FindWindowsSdkVersion()
	{
		const std::optional<std::filesystem::path> programFiles = ResolveProgramFilesX86();
		if (!programFiles.has_value())
		{
			return std::nullopt;
		}

		const std::filesystem::path includeRoot = *programFiles / "Windows Kits" / "10" / "Include";
		std::error_code errorCode;
		if (!std::filesystem::is_directory(includeRoot, errorCode))
		{
			return std::nullopt;
		}

		std::optional<std::string> latestVersion;
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(includeRoot, errorCode))
		{
			if (!entry.is_directory(errorCode))
			{
				continue;
			}

			const std::string version = entry.path().filename().string();
			if (!latestVersion.has_value() || version > *latestVersion)
			{
				latestVersion = version;
			}
		}
		return latestVersion;
	}

	VisualStudioToolchainDiscovery DiscoverVisualStudioToolchain()
	{
		VisualStudioToolchainDiscovery discovery;
		discovery.DiscoveryPath = ResolveVswherePath().value_or(std::filesystem::path());
		discovery.InstallationPath = FindVisualStudioInstallWithCppTools().value_or(
		    QueryVisualStudioInstallWithCppTools(discovery.DiscoveryPath).value_or(std::filesystem::path()));
		discovery.IdePath = FindVisualStudioIde(discovery.InstallationPath).value_or(std::filesystem::path());
		discovery.InstallerPath = ResolveVisualStudioInstallerPath().value_or(std::filesystem::path());
		discovery.ClangClPath = FindVisualStudioClangCl(discovery.InstallationPath).value_or(std::filesystem::path());
		discovery.WindowsSdkVersion = FindWindowsSdkVersion().value_or(std::string());
		return discovery;
	}

	std::string ResolveVisualStudioGenerator()
	{
		for (const std::filesystem::path& root : GetProgramFilesRoots())
		{
			const std::filesystem::path visualStudioRoot = root / "Microsoft Visual Studio";
			std::error_code errorCode;
			if (std::filesystem::exists(visualStudioRoot / "18", errorCode)
			    || std::filesystem::exists(visualStudioRoot / "2026", errorCode))
			{
				return "Visual Studio 18 2026";
			}
		}
		return "Visual Studio 17 2022";
	}
}
