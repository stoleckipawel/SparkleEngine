#pragma once

#include "SparkleCliArguments.h"
#include "SparkleCliOutput.h"
#include "SparkleLauncher/ProjectDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	class SparkleCliDispatcher final
	{
	public:
		int Dispatch(const SparkleCliArguments& arguments, std::ostream& output, std::ostream& error) const;

	private:
		std::string ChooseProjectId(const std::vector<SparkleProject>& projects, std::string_view requestedProjectId) const;
		int DispatchBuild(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const;
		int DispatchCook(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const;
		int DispatchMaintenance(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const;
		int DispatchLaunch(const SparkleCliArguments& arguments, const RepositoryRoot& repository, std::string_view projectId, std::ostream& output) const;
		int FinishDryRun(bool canRun) const;
		int FinishOperation(const OperationRecord& operation) const;

		SparkleCliOutput Output;
	};
}