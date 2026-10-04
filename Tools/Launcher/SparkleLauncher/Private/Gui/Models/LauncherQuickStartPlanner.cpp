#include "LauncherQuickStartPlanner.h"

#include "LauncherCapabilityProviders.h"
#include "LauncherOperationRequestFactory.h"

namespace SparkleLauncher
{
	static constexpr LauncherCapabilityProvider kQuickStartCapabilityProviders[] = {
	    RegisterHostToolCapabilities,
	    RegisterSourceDependencyCapabilities,
	    RegisterWorkspaceCapabilities,
	    RegisterLevelCapabilities,
	    RegisterCookCapabilities,
	    RegisterLevelRunCapabilities};

	LauncherCapabilityResolution PlanLauncherQuickStartStep(
	    const LauncherOperationRequest& launchRequest,
	    const LauncherLevelUiModel& levelModel)
	{
		LauncherCapabilityRegistry registry;
		const LauncherCapabilityContext context{BuildQuickStartOperationRequest(launchRequest, launchRequest.OperationId), levelModel};
		for (const LauncherCapabilityProvider provider : kQuickStartCapabilityProviders)
		{
			const std::string registrationError = provider(registry, context);
			if (!registrationError.empty())
			{
				LauncherCapabilityResolution resolution;
				resolution.StatusMessage = registrationError;
				return resolution;
			}
		}

		return registry.Resolve(context.Request.OperationId.toStdString());
	}
}
