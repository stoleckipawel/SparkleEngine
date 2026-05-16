#pragma once

struct FrameGraphPlan;

namespace FrameGraphPlanDiagnostics
{
	void LogIfEnabled(const FrameGraphPlan& plan) noexcept;
}
