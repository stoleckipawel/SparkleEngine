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
		case BuildWorkspaceOperationKind::SetupWorkspace:
			return "SetupWorkspace";
		case BuildWorkspaceOperationKind::GenerateSolution:
			return "GenerateSolution";
		case BuildWorkspaceOperationKind::CompileEditor:
			return "CompileEditor";
		case BuildWorkspaceOperationKind::CompileRuntime:
			return "CompileRuntime";
		case BuildWorkspaceOperationKind::BuildCookTools:
			return "BuildCookTools";
		}

		return "Unknown";
	}
}