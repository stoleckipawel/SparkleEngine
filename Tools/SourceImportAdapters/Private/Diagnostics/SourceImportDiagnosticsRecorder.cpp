#include "PCH.h"

#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include "SourceImportResult.h"

void SourceImportDiagnosticsRecorder::RecordSourceSummary(
    SourceImportResult& result,
    std::size_t sourceMeshCount,
    std::size_t sourceMaterialCount) noexcept
{
	result.diagnostics.summary.sourceMeshCount = sourceMeshCount;
	result.diagnostics.summary.sourceMaterialCount = sourceMaterialCount;
}

void SourceImportDiagnosticsRecorder::RecordSceneFeatures(
    SourceImportResult& result,
    const SourceSceneFeatureDiagnostics& diagnostics) noexcept
{
	result.diagnostics.sceneFeatures = diagnostics;
}

void SourceImportDiagnosticsRecorder::RecordImportedScenePayload(SourceImportResult& result) noexcept
{
	result.diagnostics.summary.importedMeshPrimitiveCount = result.scene.meshPrimitives.size();
	result.diagnostics.summary.importedMeshInstanceCount = result.scene.meshInstances.size();
	result.diagnostics.summary.importedMaterialCount = result.scene.materials.size();

	std::size_t resolvedTextureBindingCount = 0;
	std::size_t alphaMaskMaterialCount = 0;
	std::size_t alphaBlendMaterialCount = 0;
	for (const ImportedMaterial& material : result.scene.materials)
	{
		resolvedTextureBindingCount += material.textureSources.size();
		alphaMaskMaterialCount += material.alphaMode == ImportedAlphaMode::Mask ? 1u : 0u;
		alphaBlendMaterialCount += material.alphaMode == ImportedAlphaMode::Blend ? 1u : 0u;
	}

	result.diagnostics.textures.resolvedTextureBindingCount = resolvedTextureBindingCount;
	result.diagnostics.materials.alphaMaskMaterialCount = alphaMaskMaterialCount;
	result.diagnostics.materials.alphaBlendMaterialCount = alphaBlendMaterialCount;
}

void SourceImportDiagnosticsRecorder::RecordGeometryInstancingBaseline(
	SourceImportResult& result,
	const SourceGeometryInstancingDiagnostics& diagnostics) noexcept
{
	result.diagnostics.geometryInstancing = diagnostics;
}

void SourceImportDiagnosticsRecorder::RecordGeometryInstancingPrimitiveCandidates(
	SourceImportResult& result,
	std::size_t candidateCount) noexcept
{
	result.diagnostics.geometryInstancing.uniqueMeshPrimitiveCandidateCount = candidateCount;
}

void SourceImportDiagnosticsRecorder::RecordGeometryInstancingPlacements(SourceImportResult& result) noexcept
{
	result.diagnostics.geometryInstancing.meshPlacementCount = result.scene.meshInstances.size();
}

void SourceImportDiagnosticsRecorder::RecordWarning(SourceImportResult& result) noexcept
{
	++result.diagnostics.issues.warningMessageCount;
}

void SourceImportDiagnosticsRecorder::RecordError(SourceImportResult& result) noexcept
{
	++result.diagnostics.issues.errorMessageCount;
}

void SourceImportDiagnosticsRecorder::RecordReferencedTextureBindings(SourceImportResult& result, std::size_t count) noexcept
{
	result.diagnostics.textures.referencedTextureBindingCount += count;
}

void SourceImportDiagnosticsRecorder::RecordDuplicateTextureBindings(SourceImportResult& result, std::size_t count) noexcept
{
	result.diagnostics.textures.duplicateTextureBindingCount += count;
	RecordWarning(result);
}

void SourceImportDiagnosticsRecorder::RecordInvalidTexturePath(SourceImportResult& result) noexcept
{
	++result.diagnostics.textures.invalidTexturePathCount;
	RecordWarning(result);
}

void SourceImportDiagnosticsRecorder::RecordEmbeddedTextureBinding(SourceImportResult& result) noexcept
{
	++result.diagnostics.textures.embeddedTextureBindingCount;
	RecordWarning(result);
}

void SourceImportDiagnosticsRecorder::RecordUnsupportedTextureBinding(SourceImportResult& result) noexcept
{
	++result.diagnostics.textures.unsupportedTextureBindingCount;
	RecordWarning(result);
}

void SourceImportDiagnosticsRecorder::RecordUnsupportedMaterialFeature(SourceImportResult& result) noexcept
{
	++result.diagnostics.materials.unsupportedFeatureMaterialCount;
	RecordWarning(result);
}