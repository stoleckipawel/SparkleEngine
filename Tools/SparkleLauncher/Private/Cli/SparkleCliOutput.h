#pragma once

#include "SparkleLauncher/OperationModel.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	class SparkleCliOutput final
	{
	public:
		void PrintUsage(std::ostream& output) const;
		void PrintOperationList(std::ostream& output) const;
		void PrintOperationRecord(const OperationRecord& operation, std::ostream& output) const;
		void PrintPlanDetails(
		    const OperationRecord& operation,
		    bool canRun,
		    const std::vector<std::string>& readinessMessages,
		    const std::vector<std::string>& plannedEffects,
		    std::ostream& output) const;
	};
}