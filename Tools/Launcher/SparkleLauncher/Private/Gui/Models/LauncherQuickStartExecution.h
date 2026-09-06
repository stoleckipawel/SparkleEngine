#pragma once

#include "LauncherCapabilityRegistry.h"

#include <QtCore/QString>

#include <cstdint>
#include <set>
#include <string>

namespace SparkleLauncher
{
	enum class LauncherQuickStartCompletion : std::uint8_t
	{
		Ignored,
		Continue,
		Completed,
		Failed
	};

	class LauncherQuickStartExecution final
	{
	public:
		explicit LauncherQuickStartExecution(LauncherOperationRequest goalRequest);

		const LauncherOperationRequest& GoalRequest() const;
		const QString& ActiveRunId() const;
		const std::set<std::string>& InvalidatedCapabilityIds() const;

		std::string BeginOperation(const QString& runId, const LauncherCapabilityResolution& resolution);
		LauncherQuickStartCompletion CompleteOperation(const QString& runId, const QString& operationId, bool succeeded);

	private:
		void ClearActiveOperation();

		LauncherOperationRequest m_goalRequest;
		QString m_activeRunId;
		QString m_activeOperationId;
		std::string m_activeCapabilityId;
		std::set<std::string> m_activeInvalidatedCapabilityIds;
		std::set<std::string> m_invalidatedCapabilityIds;
		std::string m_lastCompletedCapabilityId;
		QString m_lastCompletedOperationId;
		bool m_activeOperationCompletesGoal = false;
	};
}
