#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace SparkleLauncher
{
	static std::string SourceDependencyCapabilityId(std::string_view dependencyId)
	{
		return "source-dependency." + std::string(dependencyId);
	}

	std::string RegisterSourceDependencyCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		const LauncherOperationRequest request = context.Request;
		const std::filesystem::path dependencyCacheRoot = GetBuildDirectory(request.RepositoryRoot) / "_deps";
		std::vector<std::string> dependencyCapabilityIds;
		std::string error;
		for (const SourceDependencyEntry& dependency : GetSourceDependencies())
		{
			if (!dependency.Enabled)
			{
				continue;
			}

			const std::string capabilityId = SourceDependencyCapabilityId(dependency.Id);
			dependencyCapabilityIds.push_back(capabilityId);
			error = registry.Register(
			    {capabilityId,
			        {std::string(LauncherCapabilityId::HostTools)},
			        [request, dependency, dependencyCacheRoot](bool)
			        {
				        if (ValidateSourceDependency(dependency, dependencyCacheRoot).Ready)
				        {
					        return LauncherCapabilityEvaluation::Ready();
				        }

				        LauncherOperationRequest syncRequest = BuildQuickStartOperationRequest(request, "workspace.sync-code");
				        syncRequest.SourceDependencyId = QString::fromStdString(dependency.Id);
				        const BuildWorkspaceOperationPlan plan = PlanBuildWorkspaceOperation(
				            "workspace.sync-code",
				            LauncherOperationRequestMapping::BuildWorkspace(syncRequest));
				        return plan.CanRun
				            ? LauncherCapabilityEvaluation::RunOperation(std::move(syncRequest))
				            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(plan.ReadinessMessages));
			        }});
			if (!error.empty())
			{
				return error;
			}
		}

		return registry.Register(
		    {std::string(LauncherCapabilityId::SourceDependencies),
		        std::move(dependencyCapabilityIds),
		        [dependencyCacheRoot](bool)
		        {
			        const SourceDependencyInventoryStatus status = InspectSourceDependencyCache(dependencyCacheRoot);
			        return status.AllEnabledDependenciesReady
			            ? LauncherCapabilityEvaluation::Ready()
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(status.ReadinessMessages));
		        }});
	}
}
