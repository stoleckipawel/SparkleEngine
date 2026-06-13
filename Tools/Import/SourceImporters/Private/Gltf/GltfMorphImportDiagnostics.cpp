#include "PCH.h"

#include "Gltf/GltfMorphImportDiagnostics.h"

#include "Diagnostics/GltfImportDiagnosticLog.h"
#include "SourceImportResult.h"

namespace
{
	bool HasImportedMorphWeights(const SourceImportResult& result) noexcept
	{
		for (const ImportedMeshInstance& meshInstance : result.scene.meshInstances)
		{
			if (!meshInstance.morphWeights.empty())
			{
				return true;
			}
		}

		return false;
	}
}

void GltfMorphImportDiagnostics::ReportUnsupportedWeightedNodes(SourceImportResult& result)
{
	const std::size_t weightedNodeCount = result.diagnostics.sceneFeatures.weightedNodeCount;
	if (weightedNodeCount == 0 || HasImportedMorphWeights(result))
	{
		return;
	}

	GltfImportDiagnosticLog::ReportIgnoredWeightedNodes(weightedNodeCount, result);
}
