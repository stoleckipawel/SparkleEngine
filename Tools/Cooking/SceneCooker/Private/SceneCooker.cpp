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
#include "SourceImportResult.h"

#include <fstream>
#include <optional>

class SceneCookerOperations final
{
  public:
	static void ResetManifest(CookedSceneBuild& build);
	static void FinalizeManifestHeader(CookedSceneBuild& build) noexcept;
	static bool ResolveSourceScenePath(
	    const std::filesystem::path& sourceScenePath,
	    std::filesystem::path& outResolvedPath,
	    std::string& outErrorMessage);
	static bool BuildSceneAssetId(
	    const std::filesystem::path& resolvedSourceScenePath,
	    std::string& outSceneAssetId,
	    std::string& outErrorMessage);
	static bool StageManifest(
	    const CookedSceneBuild& build,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static bool StageRegistry(
	    std::span<const CookedSceneBuild* const> builds,
	    std::vector<Files::FilePublication>& outPublication,
	    std::string& outErrorMessage);
	static bool ResolveManifestRelativePath(
	    const CookedSceneBuild& build,
	    std::filesystem::path& outRelativePath,
	    std::string& outErrorMessage);
};

void SceneCookerOperations::ResetManifest(CookedSceneBuild& build)
{
	build.manifest.instances.clear();
	build.manifest.instanceGroups.clear();
	build.manifest.morphWeights.clear();
	build.manifest.materialVariants.clear();
	build.manifest.materialVariantMappings.clear();
}

void SceneCookerOperations::FinalizeManifestHeader(
    CookedSceneBuild& build) noexcept
{
	Assets::CookedSceneManifestHeader& header = build.manifest.header;
	header.meshAssetReferenceCount =
	    static_cast<std::uint32_t>(build.manifest.meshAssetReferences.size());
	header.materialAssetReferenceCount =
	    static_cast<std::uint32_t>(build.manifest.materialAssetReferences.size());
	header.instanceCount =
	    static_cast<std::uint32_t>(build.manifest.instances.size());
	header.instanceGroupCount =
	    static_cast<std::uint32_t>(build.manifest.instanceGroups.size());
	header.cameraCount =
	    static_cast<std::uint32_t>(build.manifest.cameras.size());
	header.lightCount =
	    static_cast<std::uint32_t>(build.manifest.lights.size());
	header.skeletonRefCount =
	    static_cast<std::uint32_t>(build.manifest.skeletonRefs.size());
	header.animationRefCount =
	    static_cast<std::uint32_t>(build.manifest.animationReferences.size());
	header.morphWeightCount =
	    static_cast<std::uint32_t>(build.manifest.morphWeights.size());
	header.materialVariantCount =
	    static_cast<std::uint32_t>(build.manifest.materialVariants.size());
	header.materialVariantMappingCount =
	    static_cast<std::uint32_t>(build.manifest.materialVariantMappings.size());
}

bool SceneCooker::ResolveSceneIdentity(
    const std::filesystem::path& sourceScenePath,
    CookedSceneIdentity& outIdentity,
    std::string& outErrorMessage)
{
	std::filesystem::path resolvedSourceScenePath;
	if (!SceneCookerOperations::ResolveSourceScenePath(
	        sourceScenePath,
	        resolvedSourceScenePath,
	        outErrorMessage))
	{
		return false;
	}

	if (!SceneCookerOperations::BuildSceneAssetId(
	        resolvedSourceScenePath,
	        outIdentity.assetId,
	        outErrorMessage))
	{
		return false;
	}

	outIdentity.manifestPath = Paths::CookedSceneManifest(outIdentity.assetId);
	outErrorMessage.clear();
	return true;
}

bool SceneCooker::BuildManifest(
    const SourceImportResult& importResult,
    CookedSceneBuild& outBuild,
    std::string& outErrorMessage)
{
	SceneCookerOperations::ResetManifest(outBuild);

	CookedSceneSkeletonBuilder::BuildSkeletons(importResult, outBuild.identity.assetId, outBuild);
	CookedAnimationAssetBuilder::Build(importResult, outBuild.identity.assetId, outBuild);
	if (!CookedSceneInstanceBuilder::BuildInstances(importResult, outBuild, outErrorMessage))
	{
		return false;
	}

	if (!CookedSceneMaterialVariantBuilder::BuildMaterialVariants(importResult, outBuild, outErrorMessage))
	{
		return false;
	}

	CookedSceneCameraBuilder::BuildCameras(importResult, outBuild);
	CookedSceneLightBuilder::BuildLights(importResult, outBuild);
	CookedSceneMetadataBuilder::BuildMetadata(importResult, outBuild);

	SceneCookerOperations::FinalizeManifestHeader(outBuild);
	outErrorMessage.clear();
	return true;
}

bool SceneCooker::StageManifestsAndRegistry(
    std::span<const CookedSceneBuild* const> builds,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	for (const CookedSceneBuild* build : builds)
	{
		if (build == nullptr ||
		    !SceneCookerOperations::StageManifest(
		        *build,
		        outPublication,
		        outErrorMessage))
		{
			return false;
		}
	}

	return SceneCookerOperations::StageRegistry(
	    builds,
	    outPublication,
	    outErrorMessage);
}

bool SceneCookerOperations::StageManifest(
    const CookedSceneBuild& build,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	const std::filesystem::path stagedPath =
	    Files::BuildTemporaryPath(build.identity.manifestPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedPath);
	outPublication.push_back({stagedPath, build.identity.manifestPath});

	std::ofstream manifestOutput;
	if (!Files::TryOpenBinaryOutput(stagedPath, manifestOutput, outErrorMessage))
	{
		return false;
	}

	if (!Files::BinaryStreamWriter::WriteValue(manifestOutput, build.manifest.header, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.meshAssetReferences, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.materialAssetReferences, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.instances, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.instanceGroups, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.cameras, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.lights, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.skeletonRefs, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.animationReferences, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.morphWeights, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.materialVariants, outErrorMessage) ||
	    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.manifest.materialVariantMappings, outErrorMessage))
	{
		return false;
	}

	if (!Files::TryCloseOutput(manifestOutput, stagedPath, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool SceneCookerOperations::ResolveSourceScenePath(
    const std::filesystem::path& sourceScenePath,
    std::filesystem::path& outResolvedPath,
    std::string& outErrorMessage)
{
	if (const auto resolvedPath = Filesystem::ResolveAssetPathNormalized(sourceScenePath, AssetType::Mesh))
	{
		outResolvedPath = *resolvedPath;
		outErrorMessage.clear();
		return true;
	}

	outErrorMessage = "Unable to resolve source scene path '" + sourceScenePath.string() + "'";
	return false;
}

bool SceneCookerOperations::BuildSceneAssetId(
    const std::filesystem::path& resolvedSourceScenePath,
    std::string& outSceneAssetId,
    std::string& outErrorMessage)
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
		outErrorMessage = "Source scene path must be under a Sparkle mesh asset root to derive a stable scene asset id: '" +
		                  resolvedSourceScenePath.string() + "'";
		return false;
	}

	outSceneAssetId = relativePath->generic_string();
	std::filesystem::path sceneAssetPath(outSceneAssetId);
	sceneAssetPath.replace_extension();
	outSceneAssetId = sceneAssetPath.generic_string();
	outErrorMessage.clear();
	return true;
}

bool SceneCookerOperations::StageRegistry(
    std::span<const CookedSceneBuild* const> builds,
    std::vector<Files::FilePublication>& outPublication,
    std::string& outErrorMessage)
{
	Assets::SceneAssetRegistry registry;
	if (!registry.Load(outErrorMessage))
	{
		return false;
	}

	for (const CookedSceneBuild* build : builds)
	{
		std::filesystem::path manifestRelativePath;
		if (build == nullptr ||
		    !ResolveManifestRelativePath(
		        *build,
		        manifestRelativePath,
		        outErrorMessage))
		{
			return false;
		}

		registry.Upsert(build->identity.assetId, std::move(manifestRelativePath));
	}

	const std::filesystem::path registryPath =
	    Filesystem::GetSceneAssetRegistryPath();
	const std::filesystem::path stagedPath =
	    Files::BuildTemporaryPath(registryPath, ".cook-generation");

	Files::CleanupTemporaryFile(stagedPath);
	outPublication.push_back({stagedPath, registryPath});

	if (!registry.Save(stagedPath, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool SceneCookerOperations::ResolveManifestRelativePath(
    const CookedSceneBuild& build,
    std::filesystem::path& outRelativePath,
    std::string& outErrorMessage)
{
	const std::optional<std::filesystem::path> relativePath =
	    Paths::TryMakeRelativeUnderRoot(
	        build.identity.manifestPath,
	        Filesystem::GetCookedSceneManifestRootPath());
	if (!relativePath)
	{
		outErrorMessage =
		    "Failed to derive a relative cooked scene manifest path for scene asset id '" +
		    build.identity.assetId + "'";
		return false;
	}

	outRelativePath = *relativePath;
	outErrorMessage.clear();
	return true;
}
