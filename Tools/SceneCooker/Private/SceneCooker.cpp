#include "PCH.h"

#include "SceneCooker.h"

#include "CookArtifactCache.h"

#include "Core/Public/Assets/AssetTypes.h"
#include "Core/Public/Files/BinaryStreamWriter.h"
#include "Core/Public/Files/FileUtils.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Assets/SceneAssetRegistry.h"

#include <fstream>
#include <optional>

static constexpr std::uint32_t kSceneCookerVersion = 1;

static Cook::CookArtifactKey BuildSceneManifestCookArtifactKey(const CookedSceneBuild& build)
{
	std::uint64_t contentHash = Hash::ContinueFnv1a64Value(Hash::kFnv64OffsetBasis, build.manifestHeader);
	contentHash = Hash::ContinueFnv1a64Vector(contentHash, build.meshAssetReferences);
	contentHash = Hash::ContinueFnv1a64Vector(contentHash, build.materialAssetReferences);
	contentHash = Hash::ContinueFnv1a64Vector(contentHash, build.instances);

	return Cook::CookArtifactKey{
	    .assetType = "SceneManifest",
	    .assetId = build.sceneAssetId,
	    .cookerName = "SceneCooker",
	    .outputPath = build.sceneManifestPath,
	    .cookedFormatVersion = Assets::kCookedSceneManifestVersion,
	    .cookerVersion = kSceneCookerVersion,
	    .sourceHash = Hash::FinalizeFnv1a64(contentHash),
	    .dependencyHash = 0,
	    .settingsHash = Cook::CookArtifactCache::ComputeSettingsHash("CookedSceneManifest")};
}

bool SceneCooker::ResolveSceneAsset(
    const std::filesystem::path& sourceScenePath,
    std::string& outSceneAssetId,
    std::filesystem::path& outSceneManifestPath,
    std::string& outErrorMessage)
{
	std::filesystem::path resolvedSourceScenePath;
	if (!ResolveSourceScenePath(sourceScenePath, resolvedSourceScenePath, outErrorMessage))
	{
		return false;
	}

	if (!BuildSceneAssetId(resolvedSourceScenePath, outSceneAssetId, outErrorMessage))
	{
		return false;
	}

	outSceneManifestPath = Paths::CookedSceneManifest(outSceneAssetId);
	outErrorMessage.clear();
	return true;
}

bool SceneCooker::BuildManifest(
    const SourceImportResult& importResult,
    CookedSceneBuild& outBuild,
    std::string& outErrorMessage)
{
	outBuild.instances.clear();
	outBuild.instances.reserve(importResult.meshes.size());

	for (std::size_t meshIndex = 0; meshIndex < importResult.meshes.size(); ++meshIndex)
	{
		const Transform instanceTransform =
		    meshIndex < importResult.transforms.size() ? importResult.transforms[meshIndex] : Transform();

		std::uint32_t materialAssetIndex = Assets::kInvalidCookedMaterialAssetIndex;
		if (meshIndex < importResult.materialHandles.size() && importResult.materialHandles[meshIndex].IsValid())
		{
			materialAssetIndex = importResult.materialHandles[meshIndex].GetIndex();
			if (materialAssetIndex >= outBuild.materialAssets.size())
			{
				outErrorMessage = "Imported mesh instance references a material index outside the imported material set";
				return false;
			}
		}

		outBuild.instances.push_back(
		    Assets::CookedSceneInstanceRecord{
		        .meshAssetIndex = static_cast<std::uint32_t>(meshIndex),
		        .materialAssetIndex = materialAssetIndex,
		        .worldTransform = instanceTransform.GetWorldMatrix4x4()});
	}

	outBuild.manifestHeader.meshAssetReferenceCount = static_cast<std::uint32_t>(outBuild.meshAssetReferences.size());
	outBuild.manifestHeader.materialAssetReferenceCount = static_cast<std::uint32_t>(outBuild.materialAssetReferences.size());
	outBuild.manifestHeader.instanceCount = static_cast<std::uint32_t>(outBuild.instances.size());
	outErrorMessage.clear();
	return true;
}

bool SceneCooker::WriteSceneManifestAndRegistry(const CookedSceneBuild& build, std::string& outErrorMessage)
{
	const Cook::CookArtifactKey manifestArtifactKey = BuildSceneManifestCookArtifactKey(build);
	bool manifestIsCurrent = false;
	manifestIsCurrent = Cook::CookArtifactCache::IsCurrent(manifestArtifactKey, outErrorMessage);
	if (!manifestIsCurrent && !outErrorMessage.empty())
	{
		return false;
	}
	if (!manifestIsCurrent)
	{
		std::ofstream manifestOutput;
		if (!Files::TryOpenBinaryOutput(build.sceneManifestPath, manifestOutput, outErrorMessage))
		{
			return false;
		}

		if (!Files::BinaryStreamWriter::WriteValue(manifestOutput, build.manifestHeader, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.meshAssetReferences, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.materialAssetReferences, outErrorMessage) ||
		    !Files::BinaryStreamWriter::WriteArray(manifestOutput, build.instances, outErrorMessage))
		{
			return false;
		}

		if (!Files::TryCloseOutput(manifestOutput, build.sceneManifestPath, outErrorMessage))
		{
			return false;
		}

		if (!Cook::CookArtifactCache::Publish(manifestArtifactKey, outErrorMessage))
		{
			return false;
		}
	}

	if (!UpdateSceneAssetRegistry(build, outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}

bool SceneCooker::ResolveSourceScenePath(
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

bool SceneCooker::BuildSceneAssetId(
    const std::filesystem::path& resolvedSourceScenePath,
    std::string& outSceneAssetId,
    std::string& outErrorMessage)
{
	const std::filesystem::path projectMeshRoot = Paths::TypedAssetRoot(AssetType::Mesh, PathRoot::Project);
	const std::filesystem::path engineMeshRoot = Paths::TypedAssetRoot(AssetType::Mesh, PathRoot::Engine);

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

bool SceneCooker::UpdateSceneAssetRegistry(const CookedSceneBuild& build, std::string& outErrorMessage)
{
	const std::filesystem::path manifestRoot = Paths::CookedSceneManifestRoot();
	const std::optional<std::filesystem::path> manifestRelativePath =
	    Paths::TryMakeRelativeUnderRoot(build.sceneManifestPath, manifestRoot);
	if (!manifestRelativePath)
	{
		outErrorMessage = "Failed to derive a relative cooked scene manifest path for scene asset id '" + build.sceneAssetId + "'";
		return false;
	}

	Assets::SceneAssetRegistry sceneAssetRegistry;
	if (!sceneAssetRegistry.Load(outErrorMessage))
	{
		return false;
	}

	sceneAssetRegistry.Upsert(build.sceneAssetId, *manifestRelativePath);
	if (!sceneAssetRegistry.Save(outErrorMessage))
	{
		return false;
	}

	outErrorMessage.clear();
	return true;
}
