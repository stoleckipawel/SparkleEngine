#include "AssetPackSyncProcessRequests.h"

#include "AssetPackSyncPlanner.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <utility>

namespace SparkleLauncher
{
	static ProcessRequest MakeSyncRequest(const BuildWorkspaceOperationPlan& plan, const ProjectAssetPack& pack)
	{
		const std::filesystem::path projectRoot = plan.RepositoryRoot / "Projects" / plan.Request.ContentId;
		const std::filesystem::path cacheRoot = GetLauncherStateDirectory(plan.RepositoryRoot) / "ContentArchives" / plan.Request.ContentId;
		const std::filesystem::path rootRelativeToExtraction = pack.rootPath.lexically_relative(pack.extractionPath);

		ProcessRequest process;
		process.WorkingDirectory = plan.RepositoryRoot;
		process.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "AssetPack-" + pack.id + ".txt");
		process.ExecutablePath = plan.Toolchain.CMakePath;
		process.Arguments = {
		    "-DSPARKLE_PACK_ID=" + pack.id,
		    "-DSPARKLE_PACK_URL=" + pack.sourceUrl,
		    "-DSPARKLE_PACK_ARCHIVE_NAME=" + pack.archiveName,
		    "-DSPARKLE_PACK_ARCHIVE_BYTES=" + std::to_string(pack.archiveBytes),
		    "-DSPARKLE_PACK_ARCHIVE_SHA256=" + pack.archiveSha256,
		    "-DSPARKLE_PACK_SOURCE_PAGE=" + pack.sourcePageUrl,
		    "-DSPARKLE_PACK_VERSION=" + pack.version,
		    "-DSPARKLE_PACK_LICENSE=" + pack.license,
		    "-DSPARKLE_PACK_PROJECT_ROOT=" + projectRoot.generic_string(),
		    "-DSPARKLE_PACK_CACHE_ROOT=" + cacheRoot.generic_string(),
		    "-DSPARKLE_PACK_EXTRACT_ROOT=" + pack.extractionPath.generic_string(),
		    "-DSPARKLE_PACK_ROOT_RELATIVE=" + rootRelativeToExtraction.generic_string(),
		    "-DSPARKLE_PACK_REQUIRED_RELATIVE=" + pack.requiredRelativePath.generic_string(),
		    "-P",
		    (plan.RepositoryRoot / "Tools" / "Launcher" / "SparkleLauncher" / "Scripts" / "SyncAssetPack.cmake").generic_string(),
		};
		return process;
	}

	void AppendAssetPackSyncProcessSteps(std::vector<BuildWorkspaceProcessStep>& steps, const BuildWorkspaceOperationPlan& plan)
	{
		const std::filesystem::path projectRoot = plan.RepositoryRoot / "Projects" / plan.Request.ContentId;
		const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(projectRoot);
		for (const std::string& packId : BuildAssetPackSyncPlan(catalog, plan.Request.RequestedLevelIds))
		{
			const ProjectAssetPack& pack = catalog.assetPacks.at(packId);
			if (catalog.IsAssetPackPayloadPresent(pack))
			{
				continue;
			}

			BuildWorkspaceProcessStep step;
			step.Id = "sync-asset-pack-" + pack.id;
			step.DisplayName = "Acquire " + pack.displayName;
			step.Request = MakeSyncRequest(plan, pack);
			steps.push_back(std::move(step));
		}
	}
}
