#include "OptionalContentSyncProcessRequests.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <set>
#include <utility>

namespace SparkleLauncher
{
	static void AppendPackAndParents(
	    const ProjectLevelCatalog& catalog,
	    const ProjectOptionalContentPack& pack,
	    std::set<std::string, std::less<>>& appendedIds,
	    std::vector<ProjectOptionalContentPack>& packs)
	{
		if (appendedIds.contains(pack.id))
		{
			return;
		}
		if (!pack.parentPackId.empty())
		{
			AppendPackAndParents(catalog, catalog.optionalContentPacks.at(pack.parentPackId), appendedIds, packs);
		}
		appendedIds.insert(pack.id);
		packs.push_back(pack);
	}

	static std::vector<ProjectOptionalContentPack> CollectPacksForSync(const BuildWorkspaceOperationPlan& plan)
	{
		const std::filesystem::path projectRoot = plan.RepositoryRoot / "Projects" / plan.Request.ProjectId;
		ProjectLevelCatalog catalog;
		try
		{
			catalog = ProjectLevelCatalogFile::Load(projectRoot);
		}
		catch (const Diagnostics::Error&)
		{
			return {};
		}

		std::set<std::string, std::less<>> appendedIds;
		std::vector<ProjectOptionalContentPack> packs;
		for (const auto& contentPack : catalog.optionalContentPacks)
		{
			const ProjectOptionalContentPack& pack = contentPack.second;
			if (pack.available && pack.downloadSupported)
			{
				AppendPackAndParents(catalog, pack, appendedIds, packs);
			}
		}

		std::erase_if(packs, [&catalog](const ProjectOptionalContentPack& pack) { return catalog.IsOptionalContentPackAcquired(pack); });
		return packs;
	}

	static ProcessRequest MakeSyncRequest(const BuildWorkspaceOperationPlan& plan, const ProjectOptionalContentPack& pack)
	{
		const std::filesystem::path projectRoot = plan.RepositoryRoot / "Projects" / plan.Request.ProjectId;
		const std::filesystem::path cacheRoot = GetLauncherStateDirectory(plan.RepositoryRoot) / "ContentArchives" / plan.Request.ProjectId;
		const std::filesystem::path rootRelativeToExtraction = pack.rootPath.lexically_relative(pack.extractionPath);

		ProcessRequest process;
		process.WorkingDirectory = plan.RepositoryRoot;
		process.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "Content-" + pack.id + ".txt");
		process.ExecutablePath = plan.Toolchain.CMakePath;
		process.Arguments = {
		    "-DSPARKLE_PACK_ID=" + pack.id,
		    "-DSPARKLE_PACK_URL=" + pack.sourceUrl,
		    "-DSPARKLE_PACK_ARCHIVE_NAME=" + pack.archiveName,
		    "-DSPARKLE_PACK_ARCHIVE_BYTES=" + std::to_string(pack.archiveBytes),
		    "-DSPARKLE_PACK_PROJECT_ROOT=" + projectRoot.generic_string(),
		    "-DSPARKLE_PACK_CACHE_ROOT=" + cacheRoot.generic_string(),
		    "-DSPARKLE_PACK_EXTRACT_ROOT=" + pack.extractionPath.generic_string(),
		    "-DSPARKLE_PACK_ROOT_RELATIVE=" + rootRelativeToExtraction.generic_string(),
		    "-DSPARKLE_PACK_REQUIRED_RELATIVE=" + pack.requiredRelativePath.generic_string(),
		    "-P",
		    (plan.RepositoryRoot / "Tools" / "Launcher" / "SparkleLauncher" / "Scripts" / "SyncOptionalContentPack.cmake").generic_string(),
		};
		return process;
	}

	void AppendOptionalContentSyncProcessSteps(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan)
	{
		for (const ProjectOptionalContentPack& pack : CollectPacksForSync(plan))
		{
			BuildWorkspaceProcessStep step;
			step.Id = "sync-content-" + pack.id;
			step.DisplayName = "Acquire " + pack.displayName;
			step.Request = MakeSyncRequest(plan, pack);
			steps.push_back(std::move(step));
		}
	}
}
