#pragma once

#include "Diagnostics/AssetCookerDiagnostics.h"
#include "Planning/ProjectCookPlan.h"

#include <cstdint>
#include <vector>

class AssetCookerSceneBatch final
{
  public:
	static bool Execute(
	    const std::vector<AssetCookerSceneEntry>& sceneEntries,
	    AssetCookerDiagnostics& diagnostics);

  private:
	struct Item;

	static std::uint32_t ResolveWorkerCount() noexcept;
	static bool BuildProducts(
	    const std::vector<AssetCookerSceneEntry>& sceneEntries,
	    std::vector<Item>& items);
	static void MergeDiagnostics(
	    std::vector<Item>& items,
	    AssetCookerDiagnostics& diagnostics);
	static bool PublishProducts(
	    std::vector<Item>& items,
	    AssetCookerDiagnostics& diagnostics);
};
