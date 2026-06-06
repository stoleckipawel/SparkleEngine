#include "PCH.h"

#include "Assets/Loaders/SceneManifestLoader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Core/Public/Files/FileUtils.h"

#include <cstddef>
#include <format>

namespace Assets
{
	bool SceneManifestLoader::Load(const std::filesystem::path& path, LoadedSceneManifest& outManifest, std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
		{
			return false;
		}

		CookedAssetByteReader reader(fileBytes);
		if (!reader.Read(outManifest.header, outErrorMessage))
		{
			return false;
		}

		if (!ValidateHeader(outManifest, outErrorMessage))
		{
			return false;
		}

		if (!reader.ReadArray(outManifest.header.meshAssetReferenceCount, outManifest.meshAssetReferences, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.materialAssetReferenceCount, outManifest.materialAssetReferences, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.instanceCount, outManifest.instances, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.instanceGroupCount, outManifest.instanceGroups, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.cameraCount, outManifest.cameras, outErrorMessage))
	{
		return false;
	}

		if (!ValidateRecords(outManifest, outErrorMessage))
		{
			return false;
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			outErrorMessage = "Cooked scene manifest contains unexpected trailing bytes";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool SceneManifestLoader::ValidateHeader(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		if (manifest.header.fileHeader.magic != kCookedSceneManifestMagic)
		{
			outErrorMessage = "Invalid cooked scene manifest magic";
			return false;
		}

		if (manifest.header.fileHeader.version != kCookedSceneManifestVersion)
		{
			outErrorMessage = std::format(
			    "Cooked scene manifest version {} is not supported by this runtime; expected version {}. Recook the scene asset.",
			    manifest.header.fileHeader.version,
			    kCookedSceneManifestVersion);
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool SceneManifestLoader::ValidateRecords(const LoadedSceneManifest& manifest, std::string& outErrorMessage)
	{
		for (std::size_t meshReferenceIndex = 0; meshReferenceIndex < manifest.meshAssetReferences.size(); ++meshReferenceIndex)
		{
			if (manifest.meshAssetReferences[meshReferenceIndex].meshAssetId == InvalidCookedAssetId)
			{
				outErrorMessage = std::format("Cooked scene manifest has an invalid mesh asset reference at index {}", meshReferenceIndex);
				return false;
			}
		}

		for (std::size_t instanceIndex = 0; instanceIndex < manifest.instances.size(); ++instanceIndex)
		{
			const CookedSceneInstanceRecord& instance = manifest.instances[instanceIndex];
			if (instance.meshAssetIndex >= manifest.meshAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance {} references mesh asset index {} but only {} mesh assets exist",
				    instanceIndex,
				    instance.meshAssetIndex,
				    manifest.meshAssetReferences.size());
				return false;
			}

			if (instance.materialAssetIndex != kInvalidCookedMaterialAssetIndex && instance.materialAssetIndex >= manifest.materialAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance {} references material asset index {} but only {} material assets exist",
				    instanceIndex,
				    instance.materialAssetIndex,
				    manifest.materialAssetReferences.size());
				return false;
			}

			if (instance.groupIndex != kInvalidCookedSceneInstanceGroupIndex && instance.groupIndex >= manifest.instanceGroups.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance {} references instance group index {} but only {} groups exist",
				    instanceIndex,
				    instance.groupIndex,
				    manifest.instanceGroups.size());
				return false;
			}
		}

		for (std::size_t groupIndex = 0; groupIndex < manifest.instanceGroups.size(); ++groupIndex)
		{
			const CookedSceneInstanceGroupRecord& group = manifest.instanceGroups[groupIndex];
			if (group.meshAssetIndex >= manifest.meshAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance group {} references mesh asset index {} but only {} mesh assets exist",
				    groupIndex,
				    group.meshAssetIndex,
				    manifest.meshAssetReferences.size());
				return false;
			}

			if (group.materialAssetIndex != kInvalidCookedMaterialAssetIndex && group.materialAssetIndex >= manifest.materialAssetReferences.size())
			{
				outErrorMessage = std::format(
				    "Cooked scene instance group {} references material asset index {} but only {} material assets exist",
				    groupIndex,
				    group.materialAssetIndex,
				    manifest.materialAssetReferences.size());
				return false;
			}

			if (group.instanceCount == 0 || group.firstInstance >= manifest.instances.size() ||
			    group.instanceCount > manifest.instances.size() - group.firstInstance)
			{
				outErrorMessage = std::format(
				    "Cooked scene instance group {} references invalid instance range first={} count={} with {} instances",
				    groupIndex,
				    group.firstInstance,
				    group.instanceCount,
				    manifest.instances.size());
				return false;
			}

			if (group.groupKind != CookedSceneInstanceGroupKind::None &&
			    group.groupKind != CookedSceneInstanceGroupKind::SharedMeshReference &&
			    group.groupKind != CookedSceneInstanceGroupKind::AuthoredInstanceGroup)
			{
				outErrorMessage = std::format("Cooked scene instance group {} uses an unknown group kind", groupIndex);
				return false;
			}

			for (std::uint32_t instanceOffset = 0; instanceOffset < group.instanceCount; ++instanceOffset)
			{
				const std::size_t instanceIndex = static_cast<std::size_t>(group.firstInstance) + instanceOffset;
				if (manifest.instances[instanceIndex].groupIndex != groupIndex)
				{
					outErrorMessage = std::format(
					    "Cooked scene instance group {} range contains instance {} with mismatched group index {}",
					    groupIndex,
					    instanceIndex,
					    manifest.instances[instanceIndex].groupIndex);
					return false;
				}
			}
		}

		for (std::size_t cameraIndex = 0; cameraIndex < manifest.cameras.size(); ++cameraIndex)
		{
			const CookedSceneCameraRecord& camera = manifest.cameras[cameraIndex];
			if (camera.projectionKind != CookedSceneCameraProjectionKind::Perspective &&
			    camera.projectionKind != CookedSceneCameraProjectionKind::Orthographic &&
			    camera.projectionKind != CookedSceneCameraProjectionKind::Unknown)
			{
				outErrorMessage = std::format("Cooked scene camera {} uses an unknown projection kind", cameraIndex);
				return false;
			}
		}

		outErrorMessage.clear();
		return true;
	}
}
