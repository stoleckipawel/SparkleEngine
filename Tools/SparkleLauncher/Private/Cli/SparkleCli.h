#pragma once

#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	struct SparkleCliArguments
	{
		std::filesystem::path StartPath;
		std::string OperationId;
		std::string ProjectId;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		std::vector<std::string> SelectedTargets;
		std::vector<std::string> ShaderPackages;
		std::vector<std::string> ValidationTargets;
		CookMode RequestedCookMode = CookMode::Incremental;
		FormatMode RequestedFormatMode = FormatMode::Check;
		CleanScope RequestedCleanScope = CleanScope::SelectedProjectCookedOutputs;
		bool DryRun = false;
		bool ForceConfigure = false;
		bool ForceRecookConfirmed = false;
		bool CleanConfirmed = false;
		bool ListOperations = false;
		bool ShowHelp = false;
	};

	class SparkleCli final
	{
	public:
		int Run(int argc, char** argv, std::ostream& output, std::ostream& error) const;

	private:
		bool ParseArguments(int argc, char** argv, SparkleCliArguments& outArguments, std::ostream& error) const;
		void PrintUsage(std::ostream& output) const;
	};
}