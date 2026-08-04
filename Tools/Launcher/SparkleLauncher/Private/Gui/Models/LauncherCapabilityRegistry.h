#pragma once

#include "CapabilityGraph.h"
#include "LauncherBackend.h"

namespace SparkleLauncher
{
	using LauncherCapabilityRegistry = CapabilityGraph<LauncherOperationRequest>;
	using LauncherCapabilityEvaluation = CapabilityEvaluation<LauncherOperationRequest>;
	using LauncherCapabilityResolution = CapabilityResolution<LauncherOperationRequest>;
}
