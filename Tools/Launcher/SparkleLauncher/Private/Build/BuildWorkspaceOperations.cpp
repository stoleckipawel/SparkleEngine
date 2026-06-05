#include "SparkleLauncher/BuildWorkspaceOperations.h"

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

	std::string ToString(BuildWorkspaceOperationKind kind)
	{
		switch (kind)
		{
		case BuildWorkspaceOperationKind::CheckToolchain:
			return "CheckToolchain";
		case BuildWorkspaceOperationKind::SyncSourceTiers:
			return "SyncSourceTiers";
		case BuildWorkspaceOperationKind::GenerateBuildFiles:
			return "GenerateBuildFiles";
		case BuildWorkspaceOperationKind::OpenIde:
			return "OpenIde";
		case BuildWorkspaceOperationKind::BuildAll:
			return "BuildAll";
		case BuildWorkspaceOperationKind::CompileLauncher:
			return "CompileLauncher";
		case BuildWorkspaceOperationKind::CompileEditor:
			return "CompileEditor";
		case BuildWorkspaceOperationKind::CompileRuntime:
			return "CompileRuntime";
		case BuildWorkspaceOperationKind::BuildCookTools:
			return "BuildCookTools";
		case BuildWorkspaceOperationKind::AssembleRelease:
			return "AssembleRelease";
		}

		return "Unknown";
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
		if (text == "visual-studio" || text == "visualstudio" || text == "vs")
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
}
