#include "PCH.h"

#include "SceneCooker.h"

#include "CookedSceneBuild.h"
#include "Features/Animations/CookedAnimationAssetBuilder.h"
#include "Features/Cameras/CookedSceneCameraBuilder.h"
#include "Features/Instances/CookedSceneInstanceBuilder.h"
#include "Features/Lights/CookedSceneLightBuilder.h"
#include "Features/MaterialVariants/CookedSceneMaterialVariantBuilder.h"
#include "Features/Metadata/CookedSceneMetadataBuilder.h"
#include "Features/Skeletons/CookedSceneSkeletonBuilder.h"
#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Assets/SceneAssetRegistry.h"
#include "SourceImportOutput.h"

#include "Core/Public/Diagnostics/Error.h"

#include <fstream>
#include <optional>

class SceneCookPipeline final
{
public:
	static void ResetManifest(CookedSceneBuild& build);
	static void FinalizeManifestHeader(CookedSceneBuild& build) noexcept;
	static std::filesystem::path ResolveSourceScenePath(const std::filesystem::path& sourceScenePath);
	static std::string BuildSceneAssetId(const std::filesystem::path& resolvedSourceScenePath);
	static void StageManifest(const CookedSceneBuild& build, std::vector<Files::FilePublication>& outPublication);
	static void StageRegistry(std::span<const CookedSceneBuild* const> builds, std::vector<Files::FilePublication>& outPublication);
	static std::filesystem::path ResolveManifestRelativePath(const CookedSceneBuild& build);
};

void SceneCookPipeline::ResetManifest(CookedSceneBuild& build)
{
	build.manifest.instances.clear();
	build.manifest.instanceGroups.clear();
	build.manifest.morphWeights.clear();
	build.manifest.materialVariants.clear();
	build.manifest.materialVariantMappings.clear();
}

void SceneCookPipeline::FinalizeManifestHeader(CookedSceneBuild& build) noexcept
{
	Assets::CookedSceneManifestHeader& header = build.manifest.header;
	header.meshAssetReferenceCount = static_cast<std::uint32_t>(build.manifest.meshAssetReferences.size());
	header.materialAssetReferenceCount = static_cast<std::uint32_t>(build.manifest.materialAssetReferences.size());
	header.instanceCount = static_cast<std::uint32_t>(build.manifest.instances.size());
	header.instanceGroupCount = static_cast<std::uint32_t>(build.manifest.instanceGroups.size());
	header.cameraCount = static_cast<std::uint32_t>(build.manifest.cameras.size());
	header.lightCount = static_cast<std::uint32_t>(build.manifest.lights.size());
	header.skeletonRefCount = static_cast<std::uint32_t>(build.manifest.skeletonRefs.size());
	header.animationRefCount = static_cast<std::uint32_t>(build.manifest.animationReferences.size());
	header.morphWeightCount = static_cast<std::uint32_t>(build.manifest.morphWeights.size());
	header.materialVariantCount = static_cast<std::uint32_t>(build.manifest.materialVariants.size());
	header.materialVariantMappingCount = static_cast<std::uint32_t>(build.manifest.materialVariantMappings.size());
}

CookedSceneIdentity SceneCooker::ResolveSceneIdentity(const std::filesystem::path& sourceScenePath)
{
	const std::filesystem::path resolvedSourceScenePath = SceneCookPipeline::ResolveSourceScenePath(sourceScenePath);
	CookedSceneIdentity identity;
	identity.assetId = SceneCookPipeline::BuildSceneAssetId(resolvedSourceScenePath);
	identity.manifestPath = Paths::CookedSceneManifest(identity.assetId);
	return identity;
}

void SceneCooker::BuildManifest(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild)
{
	SceneCookPipeline::ResetManifest(outBuild);

	CookedSceneSkeletonBuilder::BuildSkeletons(importOutput, outBuild.identity.assetId, outBuild);
	CookedAnimationAssetBuilder::Build(importOutput, outBuild.identity.assetId, outBuild);
	CookedSceneInstanceBuilder::BuildInstances(importOutput, outBuild);
	CookedSceneMaterialVariantBuilder::BuildMaterialVariants(importOutput, outBuild);
	CookedSceneCameraBuilder::BuildCameras(importOutput, outBuild);
	CookedSceneLightBuilder::BuildLights(importOutput, outBuild);
	CookedSceneMetadataBuilder::BuildMetadata(importOutput, outBuild);

	SceneCookPipeline::FinalizeManifestHeader(outBuild);
}

void SceneCooker::StageManifestsAndRegistry(
    std::span<const CookedSceneBuild* const> builds,
    std::vector<Files::FilePublication>& outPublication)
{
	for (const CookedSceneBuild* build : builds)
	{
		SceneCookPipeline::StageManifest(*build, outPublication);
	}

	SceneCookPipeline::StageRegistry(builds, outPublication);
}

void SceneCookPipeline::StageManifest(const CookedSceneBuild& build, std::vector<Files::FilePublication>& outPublication)
{
	const std::filesystem::path stagedPath = Files::BuildTemporaryPath(build.identity.manifestPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedPath);
	outPublication.push_back({stagedPath, build.identity.manifestPath});

	std::ofstream manifestOutput;
	std::string errorMessage;
	if (!Files::TryOpenBinaryOutput(stagedPath, manifestOutput, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}

	if (!Files::BinaryStreamWriter::WriteValue(manifestOutput, build.manifest.header, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.meshAssetReferences, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.materialAssetReferences, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.instances, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.instanceGroups, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.cameras, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.lights, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.skeletonRefs, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.animationReferences, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.morphWeights, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.materialVariants, errorMessage)
	    || !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.materialVariantMappings, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}

	if (!Files::TryCloseOutput(manifestOutput, stagedPath, errorMessage))
	{
		throw Diagnostics::Error(errorMessage);
	}
}

std::filesystem::path SceneCookPipeline::ResolveSourceScenePath(const std::filesystem::path& sourceScenePath)
{
	if (const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(sourceScenePath, AssetType::Mesh))
	{
		return *resolvedPath;
	}

	throw Diagnostics::Error("Unable to resolve source scene path '" + sourceScenePath.string() + "'.");
}

std::string SceneCookPipeline::BuildSceneAssetId(const std::filesystem::path& resolvedSourceScenePath)
{
	const std::filesystem::path projectMeshRoot = Filesystem::GetTypedPath(AssetType::Mesh, PathRoot::Project);
	const std::filesystem::path engineMeshRoot = Filesystem::GetTypedPath(AssetType::Mesh, PathRoot::Engine);

	std::optional<std::filesystem::path> relativePath = Paths::TryMakeRelativeUnderRoot(resolvedSourceScenePath, projectMeshRoot);
	if (!relativePath)
	{
		relativePath = Paths::TryMakeRelativeUnderRoot(resolvedSourceScenePath, engineMeshRoot);
	}

	if (!relativePath)
	{
		throw Diagnostics::Error(
		    "Source scene path must be under a Sparkle mesh asset root to derive a stable scene asset id: '"
		    + resolvedSourceScenePath.string() + "'.");
	}

	std::filesystem::path sceneAssetPath(relativePath->generic_string());
	sceneAssetPath.replace_extension();
	return sceneAssetPath.generic_string();
}

void SceneCookPipeline::StageRegistry(std::span<const CookedSceneBuild* const> builds, std::vector<Files::FilePublication>& outPublication)
{
	Assets::SceneAssetRegistry registry;
	registry.Load();

	for (const CookedSceneBuild* build : builds)
	{
		registry.Upsert(build->identity.assetId, ResolveManifestRelativePath(*build));
	}

	const std::filesystem::path& registryPath = Filesystem::GetSceneAssetRegistryPath();
	const std::filesystem::path stagedPath = Files::BuildTemporaryPath(registryPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedPath);
	outPublication.push_back({stagedPath, registryPath});

	registry.Save(stagedPath);
}

std::filesystem::path SceneCookPipeline::ResolveManifestRelativePath(const CookedSceneBuild& build)
{
	const std::optional<std::filesystem::path> relativePath =
	    Paths::TryMakeRelativeUnderRoot(build.identity.manifestPath, Filesystem::GetCookedSceneManifestRootPath());
	if (!relativePath)
	{
		throw Diagnostics::Error(
		    "Failed to derive a relative cooked scene manifest path for scene asset id '" + build.identity.assetId + "'.");
	}

	return *relativePath;
}
