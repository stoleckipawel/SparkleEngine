#include "LauncherQuickStartExecution.h"

#include <utility>

namespace SparkleLauncher
{
	LauncherQuickStartExecution::LauncherQuickStartExecution(LauncherOperationRequest goalRequest) :
	    m_goalRequest(std::move(goalRequest))
	{
	}

	const LauncherOperationRequest& LauncherQuickStartExecution::GoalRequest() const
	{
		return m_goalRequest;
	}

	const QString& LauncherQuickStartExecution::ActiveRunId() const
	{
		return m_activeRunId;
	}

	const std::set<std::string>& LauncherQuickStartExecution::InvalidatedCapabilityIds() const
	{
		return m_invalidatedCapabilityIds;
	}

	std::string LauncherQuickStartExecution::BeginOperation(const QString& runId, const LauncherCapabilityResolution& resolution)
	{
		if (!m_activeRunId.isEmpty())
		{
			return "Quick Start already has an active operation.";
		}
		if (runId.isEmpty())
		{
			return "Quick Start cannot begin an operation without a run id.";
		}
		if (resolution.Result != LauncherCapabilityResolution::Kind::RunOperation || !resolution.OperationRequest.has_value()
		    || resolution.OperationRequest->OperationId.isEmpty())
		{
			return "Quick Start received an invalid operation step from the capability graph.";
		}
		if (m_lastCompletedCapabilityId == resolution.CapabilityId
		    && m_lastCompletedOperationId == resolution.OperationRequest->OperationId)
		{
			return "Operation " + resolution.OperationRequest->OperationId.toStdString() + " completed successfully, but capability "
			    + resolution.CapabilityId + " did not become ready.";
		}

		for (const std::string& capabilityId : resolution.RevalidatedCapabilityIds)
		{
			m_invalidatedCapabilityIds.erase(capabilityId);
		}
		m_activeRunId = runId;
		m_activeOperationId = resolution.OperationRequest->OperationId;
		m_activeCapabilityId = resolution.CapabilityId;
		m_activeInvalidatedCapabilityIds = resolution.InvalidatedCapabilityIds;
		m_activeOperationCompletesGoal = resolution.CompletesGoal;
		return {};
	}

	LauncherQuickStartCompletion LauncherQuickStartExecution::CompleteOperation(
	    const QString& runId,
	    const QString& operationId,
	    bool succeeded)
	{
		if (m_activeRunId != runId || m_activeOperationId != operationId)
		{
			return LauncherQuickStartCompletion::Ignored;
		}
		if (!succeeded)
		{
			ClearActiveOperation();
			return LauncherQuickStartCompletion::Failed;
		}

		m_invalidatedCapabilityIds.erase(m_activeCapabilityId);
		m_invalidatedCapabilityIds.insert(m_activeInvalidatedCapabilityIds.begin(), m_activeInvalidatedCapabilityIds.end());
		m_lastCompletedCapabilityId = m_activeCapabilityId;
		m_lastCompletedOperationId = m_activeOperationId;
		const bool completed = m_activeOperationCompletesGoal;
		ClearActiveOperation();
		return completed ? LauncherQuickStartCompletion::Completed : LauncherQuickStartCompletion::Continue;
	}

	void LauncherQuickStartExecution::ClearActiveOperation()
	{
		m_activeRunId.clear();
		m_activeOperationId.clear();
		m_activeCapabilityId.clear();
		m_activeInvalidatedCapabilityIds.clear();
		m_activeOperationCompletesGoal = false;
	}
}
