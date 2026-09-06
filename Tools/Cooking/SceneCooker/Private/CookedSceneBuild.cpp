#include "PCH.h"

#include "CookedSceneBuild.h"
#include "CookedMaterialAssetBuild.h"
#include "CookedMeshAssetBuild.h"

#include <utility>

void CookedSceneBuild::ApplyMeshOutput(MeshCookOutput&& meshOutput)
{
	manifest.meshAssetReferences = std::move(meshOutput.assetReferences);
	outputs.meshAssets = std::move(meshOutput.assets);
}

void CookedSceneBuild::ApplyMaterialOutput(MaterialCookOutput&& materialOutput)
{
	manifest.materialAssetReferences = std::move(materialOutput.assetReferences);
	outputs.materialAssets = std::move(materialOutput.assets);
}
