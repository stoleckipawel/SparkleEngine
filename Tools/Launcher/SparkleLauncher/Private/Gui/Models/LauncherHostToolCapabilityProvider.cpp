#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace SparkleLauncher
{
	static std::string HostToolCapabilityId(std::string_view toolId)
	{
		return "host-tool." + std::string(toolId);
	}

	std::string RegisterHostToolCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		const LauncherOperationRequest request = context.Request;
		const BuildWorkspaceOperationRequest workspaceRequest = LauncherOperationRequestMapping::BuildWorkspace(request);
		const BuildToolchainStatus toolchain =
		    DetectBuildToolchain(workspaceRequest.RepositoryRoot, workspaceRequest.PreferredIde, workspaceRequest.Compiler);
		std::vector<std::string> requiredCapabilityIds;
		std::string error;
		for (const ToolchainItemStatus& item : toolchain.Items)
		{
			if (!item.Required)
			{
				continue;
			}

			const std::string capabilityId = HostToolCapabilityId(item.Id);
			requiredCapabilityIds.push_back(capabilityId);
			error = registry.Register(
			    {capabilityId,
			        {},
			        [request, item](bool)
			        {
				        if (item.State == ToolchainItemState::Found)
				        {
					        return LauncherCapabilityEvaluation::Ready();
				        }
				        if (item.CanInstall)
				        {
					        LauncherOperationRequest installRequest =
					            BuildQuickStartOperationRequest(request, "workspace.install-host-tool");
					        installRequest.HostToolId = QString::fromStdString(item.Id);
					        const BuildWorkspaceOperationPlan installPlan = PlanBuildWorkspaceOperation(
					            "workspace.install-host-tool",
					            LauncherOperationRequestMapping::BuildWorkspace(installRequest));
					        if (installPlan.CanRun)
					        {
						        return LauncherCapabilityEvaluation::RunOperation(std::move(installRequest));
					        }
				        }
				        return LauncherCapabilityEvaluation::Blocked(
				            item.Detail.empty() ? item.DisplayName + " is unavailable." : item.Detail);
			        }});
			if (!error.empty())
			{
				return error;
			}
		}

		return registry.Register(
		    {std::string(LauncherCapabilityId::HostTools),
		        std::move(requiredCapabilityIds),
		        [toolchain](bool)
		        {
			        return toolchain.RequiredToolsAvailable && toolchain.ConfigurePrerequisitesAvailable
			            ? LauncherCapabilityEvaluation::Ready()
			            : LauncherCapabilityEvaluation::DependenciesRequired("Required host tools are incomplete.");
		        }});
	}
}
