#include "AssetPackSyncPlanner.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <set>
#include <utility>

namespace SparkleLauncher
{
	class AssetPackSyncPlanner final
	{
	public:
		AssetPackSyncPlanner(const ProjectLevelCatalog& catalog, BuildWorkspaceOperationKind operationKind) noexcept :
		    m_catalog(catalog),
		    m_operationKind(operationKind)
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
			for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
			{
				if (!level.selected || level.assetPackId.empty())
				{
					continue;
				}

				const ProjectAssetPack& pack = m_catalog.assetPacks.at(level.assetPackId);
				if (!pack.runtimeSupported)
				{
					throw Diagnostics::Error(
					    "Selected level '" + level.id + "' requires runtime-unsupported asset pack '" + pack.id
					    + "': " + pack.runtimeBlocker);
				}
				AppendPackAndParents(pack);
			}
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
		std::set<std::string, std::less<>> m_appendedIds;
		std::vector<std::string> m_packIds;
	};

	std::vector<std::string> BuildAssetPackSyncPlan(const ProjectLevelCatalog& catalog, BuildWorkspaceOperationKind operationKind)
	{
		return AssetPackSyncPlanner(catalog, operationKind).Build();
	}
}
