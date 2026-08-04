#include "LauncherCapabilityProviders.h"

#include "LauncherOperationRequestFactory.h"
#include "LauncherOperationRequestMapping.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace SparkleLauncher
{
	static LaunchOperationPlan PlanRequestedProductLaunch(const LauncherOperationRequest& request)
	{
		return PlanLaunchOperation(request.OperationId.toStdString(), LauncherOperationRequestMapping::Launch(request));
	}

	static std::string SourceDependencyCapabilityId(std::string_view dependencyId)
	{
		return "source-dependency." + std::string(dependencyId);
	}

	std::string RegisterWorkspaceCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context)
	{
		const LauncherOperationRequest request = context.Request;
		std::string error = registry.Register(
		    {std::string(LauncherCapabilityId::HostTools),
		        {},
		        [request](bool)
		        {
			        const BuildWorkspaceOperationPlan plan =
			            PlanBuildWorkspaceOperation("workspace.sync-code", LauncherOperationRequestMapping::BuildWorkspace(request));
			        if (plan.Toolchain.RequiredToolsAvailable && plan.Toolchain.ConfigurePrerequisitesAvailable)
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }
			        return LauncherCapabilityEvaluation::Blocked(BuildCapabilityReadinessSummary(plan.ReadinessMessages));
		        }});
		if (!error.empty())
		{
			return error;
		}

		const std::filesystem::path dependencyCacheRoot = GetBuildDirectory(request.RepositoryRoot) / "_deps";
		std::vector<std::string> sourceDependencyCapabilityIds;
		for (const SourceDependencyEntry& dependency : GetSourceDependencies())
		{
			if (!dependency.Enabled)
			{
				continue;
			}

			const std::string capabilityId = SourceDependencyCapabilityId(dependency.Id);
			sourceDependencyCapabilityIds.push_back(capabilityId);
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

		error = registry.Register(
		    {std::string(LauncherCapabilityId::SourceDependencies),
		        std::move(sourceDependencyCapabilityIds),
		        [dependencyCacheRoot](bool)
		        {
			        const SourceDependencyInventoryStatus status = InspectSourceDependencyCache(dependencyCacheRoot);
			        return status.AllEnabledDependenciesReady
			            ? LauncherCapabilityEvaluation::Ready()
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(status.ReadinessMessages));
		        }});
		if (!error.empty())
		{
			return error;
		}

		error = registry.Register(
		    {std::string(LauncherCapabilityId::BuildFiles),
		        {std::string(LauncherCapabilityId::SourceDependencies)},
		        [request, buildOperationId = context.ProductBuildOperationId()](bool)
		        {
			        const BuildWorkspaceOperationRequest workspaceRequest = LauncherOperationRequestMapping::BuildWorkspace(request);
			        const BuildWorkspaceOperationPlan productPlan =
			            PlanBuildWorkspaceOperation(buildOperationId.toStdString(), workspaceRequest);
			        if (productPlan.Freshness.Current)
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }

			        const BuildWorkspaceOperationPlan generatePlan =
			            PlanBuildWorkspaceOperation("workspace.generate-build-files", workspaceRequest);
			        return generatePlan.CanRun ? LauncherCapabilityEvaluation::RunOperation(
			                                         BuildQuickStartOperationRequest(request, "workspace.generate-build-files"))
			                                   : LauncherCapabilityEvaluation::DependenciesRequired(
			                                         BuildCapabilityReadinessSummary(generatePlan.ReadinessMessages));
		        }});
		if (!error.empty())
		{
			return error;
		}

		return registry.Register(
		    {context.ProductCapabilityId(),
		        {std::string(LauncherCapabilityId::BuildFiles)},
		        [request, buildOperationId = context.ProductBuildOperationId()](bool)
		        {
			        if (PlanRequestedProductLaunch(request).Readiness.ExecutableReady)
			        {
				        return LauncherCapabilityEvaluation::Ready();
			        }

			        const BuildWorkspaceOperationPlan buildPlan = PlanBuildWorkspaceOperation(
			            buildOperationId.toStdString(),
			            LauncherOperationRequestMapping::BuildWorkspace(request));
			        return buildPlan.CanRun
			            ? LauncherCapabilityEvaluation::RunOperation(BuildQuickStartOperationRequest(request, buildOperationId))
			            : LauncherCapabilityEvaluation::DependenciesRequired(BuildCapabilityReadinessSummary(buildPlan.ReadinessMessages));
		        }});
	}
}
