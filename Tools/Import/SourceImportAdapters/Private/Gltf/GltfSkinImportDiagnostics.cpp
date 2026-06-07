#include "PCH.h"

#include "Gltf/GltfSkinImportDiagnostics.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"

#include <cstdint>
#include <unordered_set>

std::size_t GltfSkinImportDiagnostics::CountSkinnedMeshNodesImportedThroughSkinPath(const SourceImportResult& result)
{
	std::unordered_set<std::uint32_t> skinnedSourceNodes;
	skinnedSourceNodes.reserve(result.scene.meshInstances.size());

	for (const ImportedMeshInstance& meshInstance : result.scene.meshInstances)
	{
		if (!meshInstance.HasPrimitiveBinding() || !meshInstance.HasSkeletonBinding() ||
		    meshInstance.primitiveIndex >= result.scene.meshPrimitives.size())
		{
			continue;
		}

		const ImportedMeshPrimitive& primitive = result.scene.meshPrimitives[meshInstance.primitiveIndex];
		if (primitive.geometry.hasSkinInfluences)
		{
			skinnedSourceNodes.insert(meshInstance.sourceNodeIndex);
		}
	}

	return skinnedSourceNodes.size();
}

void GltfSkinImportDiagnostics::ReportStaticOnlySkinnedNodes(SourceImportResult& result)
{
	const std::size_t importedSkinnedNodeCount = CountSkinnedMeshNodesImportedThroughSkinPath(result);
	if (result.diagnostics.sceneFeatures.skinnedNodeCount > importedSkinnedNodeCount)
	{
		GltfImportDiagnosticLog::ReportStaticSkinnedNodes(result.diagnostics.sceneFeatures.skinnedNodeCount - importedSkinnedNodeCount, result);
	}
}
