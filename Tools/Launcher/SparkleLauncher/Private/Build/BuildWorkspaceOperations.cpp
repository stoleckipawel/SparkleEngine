#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include "HostGraphicsCapabilities.h"

namespace SparkleLauncher
{
	std::string ToString(ToolchainItemState state)
	{
		switch (state)
		{
			case ToolchainItemState::Found:
				return "Found";
			case ToolchainItemState::Missing:
				return "Missing";
			case ToolchainItemState::Warning:
				return "Warning";
		}

		return "Unknown";
	}

	std::string ToString(BuildFilesFreshnessState state)
	{
		switch (state)
		{
			case BuildFilesFreshnessState::Current:
				return "Current";
			case BuildFilesFreshnessState::BuildDirectoryMissing:
				return "BuildDirectoryMissing";
			case BuildFilesFreshnessState::CMakeCacheMissing:
				return "CMakeCacheMissing";
			case BuildFilesFreshnessState::SolutionMissing:
				return "SolutionMissing";
			case BuildFilesFreshnessState::GeneratorMismatch:
				return "GeneratorMismatch";
			case BuildFilesFreshnessState::FeatureSetMismatch:
				return "FeatureSetMismatch";
			case BuildFilesFreshnessState::FreshnessStampMissing:
				return "FreshnessStampMissing";
			case BuildFilesFreshnessState::FreshnessStampMismatch:
				return "FreshnessStampMismatch";
			case BuildFilesFreshnessState::SourceListChanged:
				return "SourceListChanged";
			case BuildFilesFreshnessState::BuildInputChanged:
				return "BuildInputChanged";
			case BuildFilesFreshnessState::Unsupported:
				return "Unsupported";
		}

		return "Unknown";
	}

	WorkspaceFeatureSettings GetLauncherWorkspaceFeatureSettings()
	{
		WorkspaceFeatureSettings settings;
		const HostGraphicsCapabilities& hostGraphics = GetHostGraphicsCapabilities();
#if SPARKLE_ENABLE_CONTENT_PIPELINE
		settings.ContentPipelineEnabled = true;
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
		settings.ShaderCompilerEnabled = true;
#endif
#if SPARKLE_ENABLE_KTX_SUPPORT
		settings.KtxSupportEnabled = true;
#endif
#if SPARKLE_ENABLE_NVIDIA_STREAMLINE
		settings.NvidiaStreamlineEnabled = hostGraphics.HasNvidiaAdapter;
#endif
		return settings;
	}

	std::string ToString(BuildWorkspaceOperationKind kind)
	{
		switch (kind)
		{
			case BuildWorkspaceOperationKind::SyncCode:
				return "SyncCode";
			case BuildWorkspaceOperationKind::GenerateBuildFiles:
				return "GenerateBuildFiles";
			case BuildWorkspaceOperationKind::BuildWorkspace:
				return "BuildWorkspace";
			case BuildWorkspaceOperationKind::CompileLauncher:
				return "CompileLauncher";
			case BuildWorkspaceOperationKind::CompileEditor:
				return "CompileEditor";
			case BuildWorkspaceOperationKind::CompileRuntime:
				return "CompileRuntime";
			case BuildWorkspaceOperationKind::BuildCookTools:
				return "BuildCookTools";
			case BuildWorkspaceOperationKind::InstallHostTool:
				return "InstallHostTool";
		}

		return "Unknown";
	}

	std::string BuildWorkspaceScopeId(BuildWorkspaceScope scope)
	{
		switch (scope)
		{
			case BuildWorkspaceScope::Editor:
				return "editor";
			case BuildWorkspaceScope::Runtime:
				return "runtime";
			case BuildWorkspaceScope::CookTools:
				return "cook-tools";
			case BuildWorkspaceScope::Launcher:
				return "launcher";
		}

		return "unknown";
	}

	bool TryParseBuildWorkspaceScope(std::string_view text, BuildWorkspaceScope& outScope)
	{
		if (text == "editor")
		{
			outScope = BuildWorkspaceScope::Editor;
			return true;
		}
		if (text == "runtime")
		{
			outScope = BuildWorkspaceScope::Runtime;
			return true;
		}
		if (text == "cook-tools")
		{
			outScope = BuildWorkspaceScope::CookTools;
			return true;
		}
		if (text == "launcher")
		{
			outScope = BuildWorkspaceScope::Launcher;
			return true;
		}
		return false;
	}

	std::string ToString(WorkspaceIde ide)
	{
		switch (ide)
		{
			case WorkspaceIde::VisualStudio:
				return "VisualStudio";
			case WorkspaceIde::Rider:
				return "Rider";
		}

		return "Unknown";
	}

	std::string DisplayName(WorkspaceIde ide)
	{
		switch (ide)
		{
			case WorkspaceIde::VisualStudio:
				return "Visual Studio";
			case WorkspaceIde::Rider:
				return "Rider";
		}

		return "Unknown";
	}

	std::string WorkspaceIdeCommandLineValue(WorkspaceIde ide)
	{
		switch (ide)
		{
			case WorkspaceIde::VisualStudio:
				return "visual-studio";
			case WorkspaceIde::Rider:
				return "rider";
		}

		return "unknown";
	}

	bool TryParseWorkspaceIde(std::string_view text, WorkspaceIde& outIde)
	{
		if (text == "visual-studio")
		{
			outIde = WorkspaceIde::VisualStudio;
			return true;
		}
		if (text == "rider")
		{
			outIde = WorkspaceIde::Rider;
			return true;
		}
		return false;
	}

	std::string ToString(WorkspaceCompiler compiler)
	{
		switch (compiler)
		{
			case WorkspaceCompiler::Msvc:
				return "MSVC";
			case WorkspaceCompiler::ClangCl:
				return "ClangCl";
		}

		return "Unknown";
	}

	std::string DisplayName(WorkspaceCompiler compiler)
	{
		switch (compiler)
		{
			case WorkspaceCompiler::Msvc:
				return "MSVC";
			case WorkspaceCompiler::ClangCl:
				return "clang-cl";
		}

		return "Unknown";
	}

	std::string WorkspaceCompilerCommandLineValue(WorkspaceCompiler compiler)
	{
		switch (compiler)
		{
			case WorkspaceCompiler::Msvc:
				return "msvc";
			case WorkspaceCompiler::ClangCl:
				return "clang-cl";
		}

		return "unknown";
	}

	bool TryParseWorkspaceCompiler(std::string_view text, WorkspaceCompiler& outCompiler)
	{
		if (text == "msvc")
		{
			outCompiler = WorkspaceCompiler::Msvc;
			return true;
		}
		if (text == "clang-cl")
		{
			outCompiler = WorkspaceCompiler::ClangCl;
			return true;
		}
		return false;
	}

	bool HasIncompleteEnabledSourceDependencies(const BuildWorkspaceOperationPlan& plan)
	{
		return plan.SourceDependencies.ReadyDependencyCount < plan.SourceDependencies.EnabledDependencyCount;
	}

	bool BuildWorkspaceOperationRequiresConfigureStep(const BuildWorkspaceOperationPlan& plan)
	{
		switch (plan.Kind)
		{
			case BuildWorkspaceOperationKind::SyncCode:
				return !plan.Request.SourceDependencyId.empty() || plan.Request.ForceConfigure || !plan.Freshness.Current
				    || HasIncompleteEnabledSourceDependencies(plan);
			case BuildWorkspaceOperationKind::GenerateBuildFiles:
				return true;
			case BuildWorkspaceOperationKind::BuildWorkspace:
				return plan.Request.ForceConfigure || !plan.Freshness.Current;
			case BuildWorkspaceOperationKind::InstallHostTool:
				return false;
			default:
				return false;
		}
	}
}
