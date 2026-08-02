#include "AssetPackSyncPlanner.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <algorithm>
#include <set>
#include <utility>

namespace SparkleLauncher
{
	class AssetPackSyncPlanner final
	{
	public:
		AssetPackSyncPlanner(
		    const ProjectLevelCatalog& catalog,
		    BuildWorkspaceOperationKind operationKind,
		    std::span<const std::string> requestedLevelIds) noexcept :
		    m_catalog(catalog),
		    m_operationKind(operationKind),
		    m_requestedLevelIds(requestedLevelIds)
		{
		}

		std::vector<std::string> Build()
		{
			if (m_operationKind == BuildWorkspaceOperationKind::SyncAll)
			{
				AppendAllDownloadablePacks();
			}
			else if (m_operationKind == BuildWorkspaceOperationKind::SyncLevels)
			{
				AppendSelectedLevelPacks();
			}
			else
			{
				throw Diagnostics::Error("Asset-pack sync planning requires a sync operation.");
			}
			return std::move(m_packIds);
		}

	private:
		void AppendAllDownloadablePacks()
		{
			for (const auto& packEntry : m_catalog.assetPacks)
			{
				const ProjectAssetPack& pack = packEntry.second;
				if (pack.downloadSupported)
				{
					AppendPackAndParents(pack);
				}
			}
		}

		void AppendSelectedLevelPacks()
		{
			if (!m_requestedLevelIds.empty())
			{
				for (const std::string& levelId : m_requestedLevelIds)
				{
					const auto level = std::find_if(
					    m_catalog.levels.begin(),
					    m_catalog.levels.end(),
					    [&levelId](const ProjectLevelCatalogEntry& candidate) { return candidate.id == levelId; });
					if (level == m_catalog.levels.end())
					{
						throw Diagnostics::Error("Requested level '" + levelId + "' is not present in the level catalog.");
					}
					AppendRequestedLevelPack(*level);
				}
				return;
			}

			for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
			{
				if (!level.selected || level.assetPackId.empty())
				{
					continue;
				}
				AppendSelectedLevelPack(level);
			}
		}

		void AppendSelectedLevelPack(const ProjectLevelCatalogEntry& level)
		{
			if (level.assetPackId.empty())
			{
				return;
			}

			const ProjectAssetPack& pack = m_catalog.assetPacks.at(level.assetPackId);
			if (!pack.runtimeSupported)
			{
				throw Diagnostics::Error(
				    "Selected level '" + level.id + "' requires runtime-unsupported asset pack '" + pack.id + "': " + pack.runtimeBlocker);
			}
			AppendPackAndParents(pack);
		}

		void AppendRequestedLevelPack(const ProjectLevelCatalogEntry& level)
		{
			if (level.assetPackId.empty())
			{
				return;
			}

			AppendPackAndParents(m_catalog.assetPacks.at(level.assetPackId));
		}

		void AppendPackAndParents(const ProjectAssetPack& pack)
		{
			if (m_appendedIds.contains(pack.id))
			{
				return;
			}
			if (!pack.downloadSupported && !m_catalog.IsAssetPackPayloadPresent(pack))
			{
				throw Diagnostics::Error("Asset pack '" + pack.id + "' is required but has no supported acquisition path.");
			}
			if (!pack.parentPackId.empty())
			{
				AppendPackAndParents(m_catalog.assetPacks.at(pack.parentPackId));
			}

			m_appendedIds.insert(pack.id);
			m_packIds.push_back(pack.id);
		}

		const ProjectLevelCatalog& m_catalog;
		BuildWorkspaceOperationKind m_operationKind;
		std::span<const std::string> m_requestedLevelIds;
		std::set<std::string, std::less<>> m_appendedIds;
		std::vector<std::string> m_packIds;
	};

	std::vector<std::string> BuildAssetPackSyncPlan(
	    const ProjectLevelCatalog& catalog,
	    BuildWorkspaceOperationKind operationKind,
	    std::span<const std::string> requestedLevelIds)
	{
		return AssetPackSyncPlanner(catalog, operationKind, requestedLevelIds).Build();
	}
}
