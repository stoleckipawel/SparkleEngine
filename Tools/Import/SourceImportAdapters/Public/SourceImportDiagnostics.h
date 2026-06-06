#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

enum class SourceImportFeatureSupport : std::uint32_t
{
	Imported = 0,
	PartiallyImported = 1,
	Unsupported = 2,
};

inline std::string_view ToString(SourceImportFeatureSupport support) noexcept
{
	switch (support)
	{
	case SourceImportFeatureSupport::Imported:
		return "imported";
	case SourceImportFeatureSupport::PartiallyImported:
		return "partially-imported";
	case SourceImportFeatureSupport::Unsupported:
		return "unsupported";
	default:
		return "unknown";
	}
}

struct SourceImportFeatureCapability
{
	std::size_t count = 0;
	SourceImportFeatureSupport support = SourceImportFeatureSupport::Unsupported;
};

struct SourceImportSummaryDiagnostics
{
	std::size_t sourceMeshCount = 0;
	std::size_t importedMeshPrimitiveCount = 0;
	std::size_t importedMeshInstanceCount = 0;
	std::size_t importedMeshInstanceGroupCount = 0;
	std::size_t sourceMaterialCount = 0;
	std::size_t importedMaterialCount = 0;
};

struct SourceSceneFeatureDiagnostics
{
	std::size_t animationCount = 0;
	std::size_t cameraNodeCount = 0;
	std::size_t lightNodeCount = 0;
	std::size_t skinnedNodeCount = 0;
	std::size_t weightedNodeCount = 0;
	std::size_t morphTargetPrimitiveCount = 0;
	std::size_t materialVariantCount = 0;
	std::size_t materialVariantPrimitiveCount = 0;
	std::size_t embeddedTextureCount = 0;
	std::size_t authoredInstancingNodeCount = 0;
};

struct SourceImportFeatureCapabilitySummary
{
	SourceImportFeatureCapability animations;
	SourceImportFeatureCapability cameraNodes;
	SourceImportFeatureCapability lightNodes;
	SourceImportFeatureCapability skinnedNodes;
	SourceImportFeatureCapability weightedNodes;
	SourceImportFeatureCapability morphTargets;
	SourceImportFeatureCapability materialVariants;
	SourceImportFeatureCapability meshGpuInstancing;
};

struct SourceMaterialImportDiagnostics
{
	std::size_t unsupportedFeatureMaterialCount = 0;
	std::size_t alphaMaskMaterialCount = 0;
	std::size_t alphaBlendMaterialCount = 0;
};

struct SourceTextureImportDiagnostics
{
	std::size_t referencedTextureBindingCount = 0;
	std::size_t resolvedTextureBindingCount = 0;
	std::size_t embeddedTextureBindingCount = 0;
	std::size_t unsupportedTextureBindingCount = 0;
	std::size_t invalidTexturePathCount = 0;
	std::size_t duplicateTextureBindingCount = 0;
};

struct SourceGeometryInstancingDiagnostics
{
	std::size_t uniqueMeshPrimitiveCandidateCount = 0;
	std::size_t meshPlacementCount = 0;
	std::size_t authoredInstanceGroupCount = 0;
};

struct SourceImportIssueDiagnostics
{
	std::size_t warningMessageCount = 0;
	std::size_t errorMessageCount = 0;
};

struct SourceImportDiagnostics
{
	SourceImportSummaryDiagnostics summary;
	SourceSceneFeatureDiagnostics sceneFeatures;
	SourceImportFeatureCapabilitySummary featureCapabilities;
	SourceMaterialImportDiagnostics materials;
	SourceTextureImportDiagnostics textures;
	SourceGeometryInstancingDiagnostics geometryInstancing;
	SourceImportIssueDiagnostics issues;
};
