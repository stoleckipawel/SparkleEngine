#include "PCH.h"

#include "Assets/Loaders/SceneManifestLoader.h"

#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/LoadedCookedAssets.h"
#include "Core/Public/Files/FileUtils.h"

namespace Engine::Assets
{
	bool SceneManifestLoader::Load(
	    const std::filesystem::path& path,
	    LoadedSceneManifest& outManifest,
	    std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Engine::Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
		{
			return false;
		}

		CookedAssetByteReader reader(fileBytes);
		if (!reader.Read(outManifest.header, outErrorMessage))
		{
			return false;
		}

		if (!HasValidHeader(outManifest))
		{
			outErrorMessage = "Invalid cooked scene manifest header";
			return false;
		}

		if (!reader.ReadArray(outManifest.header.meshAssetReferenceCount, outManifest.meshAssetReferences, outErrorMessage) ||
		    !reader.ReadArray(
		        outManifest.header.materialAssetReferenceCount,
		        outManifest.materialAssetReferences,
		        outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.instanceCount, outManifest.instances, outErrorMessage))
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

	bool SceneManifestLoader::HasValidHeader(const LoadedSceneManifest& manifest) noexcept
	{
		return manifest.header.fileHeader.Matches(kCookedSceneManifestMagic, kCookedSceneManifestVersion);
	}
}