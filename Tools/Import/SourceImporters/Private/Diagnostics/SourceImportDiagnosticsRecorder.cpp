#include "PCH.h"

#include "Diagnostics/SourceImportDiagnosticsRecorder.h"

#include "SourceImportResult.h"

namespace
{
	SourceImportFeatureCapability BuildFeatureCapability(
	    std::size_t count,
	    SourceImportFeatureSupport presentFeatureSupport) noexcept
	{
		return {
		    count,
		    count == 0 ? SourceImportFeatureSupport::NotPresent : presentFeatureSupport};
	}
}

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
	result.diagnostics.featureCapabilities.animations =
	    BuildFeatureCapability(diagnostics.animationCount, SourceImportFeatureSupport::Unsupported);
	result.diagnostics.featureCapabilities.cameraNodes =
	    BuildFeatureCapability(diagnostics.cameraNodeCount, SourceImportFeatureSupport::Unsupported);
	result.diagnostics.featureCapabilities.lightNodes =
	    BuildFeatureCapability(diagnostics.lightNodeCount, SourceImportFeatureSupport::Unsupported);
	result.diagnostics.featureCapabilities.skinnedNodes =
	    BuildFeatureCapability(diagnostics.skinnedNodeCount, SourceImportFeatureSupport::PartiallyImported);
	result.diagnostics.featureCapabilities.weightedNodes =
	    BuildFeatureCapability(diagnostics.weightedNodeCount, SourceImportFeatureSupport::Unsupported);
	result.diagnostics.featureCapabilities.morphTargets =
	    BuildFeatureCapability(diagnostics.morphTargetPrimitiveCount, SourceImportFeatureSupport::Unsupported);
	result.diagnostics.featureCapabilities.materialVariants = BuildFeatureCapability(
	    diagnostics.materialVariantCount + diagnostics.materialVariantPrimitiveCount,
	    SourceImportFeatureSupport::Unsupported);
	result.diagnostics.featureCapabilities.meshGpuInstancing =
	    BuildFeatureCapability(diagnostics.authoredInstancingNodeCount, SourceImportFeatureSupport::PartiallyImported);
}

void SourceImportDiagnosticsRecorder::RecordImportedScenePayload(SourceImportResult& result) noexcept
{
	result.diagnostics.summary.importedMeshPrimitiveCount = result.scene.meshPrimitives.size();
	result.diagnostics.summary.importedMeshInstanceCount = result.scene.meshInstances.size();
	result.diagnostics.summary.importedMeshInstanceGroupCount = result.scene.meshInstanceGroups.size();
	result.diagnostics.summary.importedMaterialCount = result.scene.materials.size();
	if (!result.scene.materialVariants.empty())
	{
		const std::size_t sourceVariantPayloadCount =
		    result.diagnostics.sceneFeatures.materialVariantCount +
		    result.diagnostics.sceneFeatures.materialVariantPrimitiveCount;
		const bool importedAllVariants =
		    result.scene.materialVariants.size() == result.diagnostics.sceneFeatures.materialVariantCount;
		const bool importedMappingsForAllMappedPrimitives =
		    result.diagnostics.sceneFeatures.materialVariantPrimitiveCount == 0 ||
		    !result.scene.materialVariantMappings.empty();
		result.diagnostics.featureCapabilities.materialVariants = {
		    sourceVariantPayloadCount,
		    importedAllVariants && importedMappingsForAllMappedPrimitives ? SourceImportFeatureSupport::Imported
		                                                                  : SourceImportFeatureSupport::PartiallyImported};
	}
	std::size_t importedMorphTargetPrimitiveCount = 0;
	for (const ImportedMeshPrimitive& primitive : result.scene.meshPrimitives)
	{
		importedMorphTargetPrimitiveCount += primitive.geometry.HasSkinInfluences() && primitive.geometry.HasMorphTargets() ? 1u : 0u;
	}
	if (importedMorphTargetPrimitiveCount > 0)
	{
		result.diagnostics.featureCapabilities.morphTargets = {
		    result.diagnostics.sceneFeatures.morphTargetPrimitiveCount,
		    SourceImportFeatureSupport::Imported};
	}
	if (result.diagnostics.sceneFeatures.weightedNodeCount > 0 && importedMorphTargetPrimitiveCount > 0)
	{
		result.diagnostics.featureCapabilities.weightedNodes = {
		    result.diagnostics.sceneFeatures.weightedNodeCount,
		    SourceImportFeatureSupport::Imported};
	}

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
	result.diagnostics.geometryInstancing.authoredInstanceGroupCount = result.scene.meshInstanceGroups.size();
	if (result.diagnostics.sceneFeatures.authoredInstancingNodeCount > 0)
	{
		const SourceImportFeatureSupport support =
		    result.scene.meshInstanceGroups.size() >= result.diagnostics.sceneFeatures.authoredInstancingNodeCount
		        ? SourceImportFeatureSupport::Imported
		        : SourceImportFeatureSupport::PartiallyImported;
		result.diagnostics.featureCapabilities.meshGpuInstancing = {
		    result.diagnostics.sceneFeatures.authoredInstancingNodeCount,
		    support};
	}
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
