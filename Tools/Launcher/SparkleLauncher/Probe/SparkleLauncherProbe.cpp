#include "CMakeWorkflowProcessRequests.h"
#include "HostToolInstaller.h"
#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ProcessRunner.h"
#include "SparkleLauncher/ContentDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <vector>

namespace SparkleLauncher
{
	static bool Contains(const std::vector<std::string>& values, std::string_view expected)
	{
		return std::find(values.begin(), values.end(), expected) != values.end();
	}

	static bool ValidateWorkspaceCompilerContract(const std::filesystem::path& repositoryRoot, std::string& errorMessage)
	{
		WorkspaceCompiler compiler = WorkspaceCompiler::Msvc;
		if (!TryParseWorkspaceCompiler("clang-cl", compiler) || compiler != WorkspaceCompiler::ClangCl
		    || WorkspaceCompilerCommandLineValue(compiler) != "clang-cl" || DisplayName(compiler) != "clang-cl")
		{
			errorMessage = "clang-cl compiler selection did not round-trip through the workspace compiler contract.";
			return false;
		}

		BuildToolchainStatus clangToolchain;
		clangToolchain.Compiler = WorkspaceCompiler::ClangCl;
		clangToolchain.Generator = "Visual Studio 18 2026";
		clangToolchain.Platform = "x64";
		clangToolchain.Toolset = "ClangCL";
		clangToolchain.CMakePath = "cmake.exe";
		clangToolchain.VisualStudioPath = "C:/Program Files/Microsoft Visual Studio/18/Community";
		clangToolchain.VisualStudioInstallerPath = "C:/Program Files (x86)/Microsoft Visual Studio/Installer/setup.exe";
		const ProcessRequest configure = MakeCMakeConfigureRequest(repositoryRoot, clangToolchain, "test.configure", "Configure.txt");
		if (!Contains(configure.Arguments, "-T") || !Contains(configure.Arguments, "ClangCL"))
		{
			errorMessage = "Launcher-owned clang-cl selection was not mapped to the CMake toolset argument.";
			return false;
		}
		if (!CanInstallHostTool("clangcl", clangToolchain) || CanInstallHostTool("unknown", clangToolchain))
		{
			errorMessage = "Host-tool installer registry reported incorrect capabilities.";
			return false;
		}

		std::string installerError;
		const std::optional<ProcessRequest> install =
		    BuildHostToolInstallRequest("clangcl", clangToolchain, repositoryRoot, "workspace.install-host-tool", installerError);
		if (!install.has_value() || !installerError.empty() || install->Arguments.empty()
		    || std::none_of(
		        install->Arguments.begin(),
		        install->Arguments.end(),
		        [](const std::string& argument)
		        { return argument.find("Microsoft.VisualStudio.Component.VC.Llvm.ClangToolset") != std::string::npos; }))
		{
			errorMessage = "clang-cl installer registration did not produce the expected Visual Studio component request.";
			return false;
		}

		BuildWorkspaceOperationRequest installRequest;
		installRequest.RepositoryRoot = repositoryRoot;
		installRequest.Compiler = WorkspaceCompiler::ClangCl;
		installRequest.HostToolId = "clangcl";
		const BuildToolchainStatus detected = DetectBuildToolchain(repositoryRoot, WorkspaceIde::VisualStudio, installRequest.Compiler);
		const auto detectedClangCl = std::find_if(
		    detected.Items.begin(),
		    detected.Items.end(),
		    [](const ToolchainItemStatus& item) { return item.Id == "clangcl"; });
		if (detectedClangCl == detected.Items.end())
		{
			errorMessage = "clang-cl was not registered in the detected toolchain inventory.";
			return false;
		}
		if (detectedClangCl->State != ToolchainItemState::Found && detectedClangCl->CanInstall)
		{
			const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation("workspace.install-host-tool", installRequest);
			if (!plan.CanRun || plan.Steps.size() != 1 || plan.Steps.front().Id != "install-host-tool")
			{
				errorMessage = "A missing installable clang-cl toolchain did not produce one launcher-owned remediation step.";
				return false;
			}
		}

		errorMessage.clear();
		return true;
	}
}

int main(int argc, char** argv)
{
	Threading::SetCurrentThreadRole("Sparkle.ToolMain");
	std::string errorMessage;
	const std::filesystem::path startPath = argc > 1 ? std::filesystem::path(argv[1]) : std::filesystem::current_path();
	const std::optional<SparkleLauncher::RepositoryRoot> repositoryRoot = SparkleLauncher::TryFindRepositoryRoot(startPath, errorMessage);
	if (!repositoryRoot.has_value())
	{
		std::cerr << errorMessage << '\n';
		return 1;
	}

	std::cout << "Repository: " << repositoryRoot->RootPath.string() << '\n';
	std::cout << "Launcher state: " << SparkleLauncher::GetLauncherStateDirectory(repositoryRoot->RootPath).string() << '\n';
	if (!SparkleLauncher::ValidateWorkspaceCompilerContract(repositoryRoot->RootPath, errorMessage))
	{
		std::cerr << errorMessage << '\n';
		return 1;
	}

	const std::optional<SparkleLauncher::SparkleContent> content =
	    SparkleLauncher::DiscoverContentRoot(repositoryRoot->RootPath, errorMessage);
	if (!content.has_value())
	{
		std::cerr << errorMessage << '\n';
		return 1;
	}

	std::cout << "Content: " << content->RootPath.string() << '\n';
	try
	{
		const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(content->RootPath);
		std::cout << "  " << catalog.levels.size() << " levels, " << catalog.assetPacks.size() << " asset packs" << '\n';
	}
	catch (const Diagnostics::Error& error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}

	std::cout << "Profiles:" << '\n';
	for (const SparkleLauncher::BuildProfile& profile : SparkleLauncher::GetBuildProfileCatalog())
	{
		std::cout << "  " << profile.Name << " [" << SparkleLauncher::ToString(profile.State) << ", "
		          << SparkleLauncher::ToString(profile.Target) << "]" << '\n';
	}

	return 0;
}
