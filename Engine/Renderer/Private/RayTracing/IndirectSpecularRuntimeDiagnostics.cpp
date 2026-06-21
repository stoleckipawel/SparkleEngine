#include "../PCH.h"
#include "RayTracing/IndirectSpecularRuntimeDiagnostics.h"

namespace
{
	IndirectSpecularRuntimeDiagnosticsSnapshot g_snapshot{};
}

namespace IndirectSpecularRuntimeDiagnostics
{
	const char* ToString(IndirectSpecularStatusReason status) noexcept
	{
		switch (status)
		{
		case IndirectSpecularStatusReason::Disabled:
			return "disabled";
		case IndirectSpecularStatusReason::Unsupported:
			return "unsupported";
		case IndirectSpecularStatusReason::MissingTlas:
			return "missing-tlas";
		case IndirectSpecularStatusReason::MissingHitData:
			return "missing-hit-data";
		case IndirectSpecularStatusReason::Running:
			return "running";
		case IndirectSpecularStatusReason::NotEvaluated:
		default:
			return "not-evaluated";
		}
	}

	IndirectSpecularRuntimeDiagnosticsSnapshot Capture() noexcept
	{
		return g_snapshot;
	}

	void Publish(const IndirectSpecularRuntimeDiagnosticsSnapshot& snapshot) noexcept
	{
		g_snapshot = snapshot;
		g_snapshot.StatusReason = ToString(g_snapshot.Status);
	}
}
