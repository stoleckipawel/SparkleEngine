#include "PCH.h"

#include "Gltf/GltfImportFeatureDiagnostics.h"

#include "SourceImportResult.h"

void GltfImportFeatureDiagnostics::RecordImportedFeatureSupport(SourceImportResult& result)
{
	if (!result.scene.cameras.empty())
	{
		result.diagnostics.featureCapabilities.cameraNodes = {
		    result.scene.cameras.size(),
		    SourceImportFeatureSupport::Imported};
	}

	if (!result.scene.lights.empty())
	{
		result.diagnostics.featureCapabilities.lightNodes = {
		    result.scene.lights.size(),
		    SourceImportFeatureSupport::Imported};
	}
}
