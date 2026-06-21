#include "../PCH.h"
#include "RayTracing/RTIndirectSpecularRuntimeDiagnostics.h"

namespace
{
	RTIndirectSpecularRuntimeDiagnosticsSnapshot g_snapshot{};
}

namespace RTIndirectSpecularRuntimeDiagnostics
{
	const char* ToString(RTIndirectSpecularStatusReason status) noexcept
	{
		switch (status)
		{
		case RTIndirectSpecularStatusReason::Disabled:
			return "disabled";
		case RTIndirectSpecularStatusReason::Unsupported:
			return "unsupported";
		case RTIndirectSpecularStatusReason::MissingTlas:
			return "missing-tlas";
		case RTIndirectSpecularStatusReason::MissingHitData:
			return "missing-hit-data";
		case RTIndirectSpecularStatusReason::Running:
			return "running";
		case RTIndirectSpecularStatusReason::NotEvaluated:
		default:
			return "not-evaluated";
		}
	}

	RTIndirectSpecularRuntimeDiagnosticsSnapshot Capture() noexcept
	{
		return g_snapshot;
	}

	void Publish(const RTIndirectSpecularRuntimeDiagnosticsSnapshot& snapshot) noexcept
	{
		g_snapshot = snapshot;
		g_snapshot.StatusReason = ToString(g_snapshot.Status);
	}
}
