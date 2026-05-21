#include "BuildWorkspaceProcessRequests.h"

#include "CMakeWorkflowProcessRequests.h"
#include "SparkleLauncher/BuildProfileCatalog.h"

#include <optional>
#include <string_view>
#include <utility>

namespace SparkleLauncher
{
	static ProcessRequest MakeConfigureRequest(const BuildWorkspaceOperationPlan& plan)
	{
		return MakeCMakeConfigureRequest(plan.RepositoryRoot, plan.Toolchain, plan.Operation.Id, "Configure.txt");
	}

	static ProcessRequest MakeBuildRequest(const BuildWorkspaceOperationPlan& plan, std::string_view profileName, const std::vector<std::string>& targets)
	{
		return MakeCMakeBuildRequest(plan.RepositoryRoot, plan.Toolchain, plan.Operation.Id, profileName, targets, "Build.txt");
	}

	static std::vector<std::string> ResolveProjectTargets(std::string_view projectId, std::string_view profileName)
	{
		const std::optional<BuildProfile> profile = FindBuildProfile(profileName);
		if (!profile.has_value())
		{
			return {};
		}
		return {BuildProjectTargetName(projectId, *profile)};
	}

	static std::vector<std::string> ResolveBuildTargets(const BuildWorkspaceOperationPlan& plan, std::string_view profileName)
	{
		if (!plan.Request.SelectedTargets.empty())
		{
			return plan.Request.SelectedTargets;
		}
		return ResolveProjectTargets(plan.Request.ProjectId, profileName);
	}

	static void AddConfigureStep(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan)
	{
		BuildWorkspaceProcessStep step;
		step.Id = "configure";
		step.DisplayName = "Generate build files";
		step.Request = MakeConfigureRequest(plan);
		step.UpdatesBuildFilesFreshness = true;
		steps.push_back(std::move(step));
	}

	static void AddBuildStep(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan, std::string_view profileName, const std::vector<std::string>& targets)
	{
		BuildWorkspaceProcessStep step;
		step.Id = "build";
		step.DisplayName = "Build targets";
		step.Request = MakeBuildRequest(plan, profileName, targets);
		steps.push_back(std::move(step));
	}

	std::vector<BuildWorkspaceProcessStep> BuildProcessStepsForPlan(const BuildWorkspaceOperationPlan& plan)
	{
		std::vector<BuildWorkspaceProcessStep> steps;
		if (!plan.Toolchain.RequiredToolsAvailable)
		{
			return steps;
		}

		const bool needsConfigure = plan.Request.ForceConfigure || !plan.Freshness.Current;
		switch (plan.Kind)
		{
		case BuildWorkspaceOperationKind::CheckToolchain:
			return steps;
		case BuildWorkspaceOperationKind::SetupWorkspace:
			if (needsConfigure)
			{
				AddConfigureStep(steps, plan);
			}
			return steps;
		case BuildWorkspaceOperationKind::GenerateSolution:
			AddConfigureStep(steps, plan);
			return steps;
		case BuildWorkspaceOperationKind::CompileEditor:
			if (needsConfigure)
			{
				AddConfigureStep(steps, plan);
			}
			AddBuildStep(steps, plan, plan.Request.EditorProfile, ResolveBuildTargets(plan, plan.Request.EditorProfile));
			return steps;
		case BuildWorkspaceOperationKind::CompileRuntime:
			if (needsConfigure)
			{
				AddConfigureStep(steps, plan);
			}
			AddBuildStep(steps, plan, plan.Request.RuntimeProfile, ResolveBuildTargets(plan, plan.Request.RuntimeProfile));
			return steps;
		case BuildWorkspaceOperationKind::BuildCookTools:
			if (needsConfigure)
			{
				AddConfigureStep(steps, plan);
			}
			AddBuildStep(steps, plan, plan.Request.EditorProfile, {"AssetCooker", "TextureCooker", "ShaderCompiler"});
			return steps;
		}

		return steps;
	}
}
