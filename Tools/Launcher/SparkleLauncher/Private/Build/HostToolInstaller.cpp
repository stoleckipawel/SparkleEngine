#include "HostToolInstaller.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "SparkleLauncher/LauncherPaths.h"

namespace SparkleLauncher
{
	static constexpr std::string_view kClangClToolId = "clangcl";
	static constexpr std::string_view kClangClVisualStudioComponent = "Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset";

	static std::string QuotePowerShellLiteral(std::string value)
	{
		std::size_t position = 0;
		while ((position = value.find('\'', position)) != std::string::npos)
		{
			value.insert(position, 1, '\'');
			position += 2;
		}
		return "'" + value + "'";
	}

	static std::filesystem::path ResolvePowerShellPath()
	{
#if defined(_WIN32)
		std::string systemRoot;
		if (Environment::TryGetVariable("SystemRoot", systemRoot))
		{
			return std::filesystem::path(systemRoot) / "System32" / "WindowsPowerShell" / "v1.0" / "powershell.exe";
		}
		return "powershell.exe";
#else
		return "pwsh";
#endif
	}

	static bool CanInstallClangCl(const BuildToolchainStatus& toolchain)
	{
		return !toolchain.VisualStudioPath.empty() && !toolchain.VisualStudioInstallerPath.empty();
	}

	static std::optional<ProcessRequest> BuildClangClInstallRequest(
	    const BuildToolchainStatus& toolchain,
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string& errorMessage)
	{
		if (!CanInstallClangCl(toolchain))
		{
			errorMessage = "Visual Studio and its installer are required before the launcher can add clang-cl.";
			return std::nullopt;
		}

		const std::string installerArguments = "modify --installPath \"" + toolchain.VisualStudioPath.string() + "\" --add "
		    + std::string(kClangClVisualStudioComponent) + " --passive --norestart";
		const std::string script = "$ErrorActionPreference='Stop'; "
		                           "Write-Output 'Requesting administrator approval for the Visual Studio clang-cl component...'; "
		                           "$process = Start-Process -FilePath "
		    + QuotePowerShellLiteral(toolchain.VisualStudioInstallerPath.string()) + " -ArgumentList "
		    + QuotePowerShellLiteral(installerArguments)
		    + " -Verb RunAs -Wait -PassThru; "
		      "if ($process.ExitCode -ne 0) { throw ('Visual Studio Installer exited with code ' + $process.ExitCode) }; "
		      "Write-Output 'Visual Studio clang-cl component installation completed.'";

		ProcessRequest request;
		request.ExecutablePath = ResolvePowerShellPath();
		request.Arguments = {"-NoLogo", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-Command", script};
		request.WorkingDirectory = repositoryRoot;
		request.LogPath = GetLauncherOperationLogPath(repositoryRoot, operationId, "InstallHostTool.txt");
		errorMessage.clear();
		return request;
	}

	const std::vector<HostToolInstallerDefinition>& GetHostToolInstallerDefinitions()
	{
		static const std::vector<HostToolInstallerDefinition> definitions = {
		    {std::string(kClangClToolId),
		        "clang-cl",
		        "Ask Windows for administrator approval, then add clang-cl through Visual Studio Installer.",
		        CanInstallClangCl,
		        BuildClangClInstallRequest},
		};
		return definitions;
	}

	const HostToolInstallerDefinition* FindHostToolInstaller(std::string_view toolId)
	{
		for (const HostToolInstallerDefinition& definition : GetHostToolInstallerDefinitions())
		{
			if (definition.Id == toolId)
			{
				return &definition;
			}
		}
		return nullptr;
	}

	bool CanInstallHostTool(std::string_view toolId, const BuildToolchainStatus& toolchain)
	{
		const HostToolInstallerDefinition* installer = FindHostToolInstaller(toolId);
		return installer != nullptr && installer->IsAvailable != nullptr && installer->IsAvailable(toolchain);
	}

	std::optional<ProcessRequest> BuildHostToolInstallRequest(
	    std::string_view toolId,
	    const BuildToolchainStatus& toolchain,
	    const std::filesystem::path& repositoryRoot,
	    std::string_view operationId,
	    std::string& errorMessage)
	{
		const HostToolInstallerDefinition* installer = FindHostToolInstaller(toolId);
		if (installer == nullptr)
		{
			errorMessage = "No launcher installer is registered for host tool '" + std::string(toolId) + "'.";
			return std::nullopt;
		}
		if (installer->IsAvailable == nullptr || installer->BuildRequest == nullptr || !installer->IsAvailable(toolchain))
		{
			errorMessage = "The registered launcher installer for " + installer->DisplayName + " is not available on this machine.";
			return std::nullopt;
		}
		return installer->BuildRequest(toolchain, repositoryRoot, operationId, errorMessage);
	}
}
