#include "PCH.h"

#include "Assets/Loaders/MaterialAssetLoader.h"

#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/LoadedCookedAssets.h"
#include "Core/Public/Files/FileUtils.h"

namespace Engine::Assets
{
	bool MaterialAssetLoader::Load(
	    const std::filesystem::path& path,
	    LoadedMaterialAsset& outMaterialAsset,
	    std::string& outErrorMessage) const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Engine::Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
		{
			return false;
		}

		CookedAssetByteReader reader(fileBytes);
		if (!reader.Read(outMaterialAsset.header, outErrorMessage))
		{
			return false;
		}

		if (!HasValidHeader(outMaterialAsset))
		{
			outErrorMessage = "Invalid cooked material asset header";
			return false;
		}

		if (!reader.ReadString(outMaterialAsset.header.nameByteCount, outMaterialAsset.name, outErrorMessage) ||
		    !reader.ReadArray(
		        outMaterialAsset.header.textureReferenceCount,
		        outMaterialAsset.textureReferences,
		        outErrorMessage))
		{
			return false;
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			outErrorMessage = "Cooked material asset contains unexpected trailing bytes";
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool MaterialAssetLoader::HasValidHeader(const LoadedMaterialAsset& materialAsset) noexcept
	{
		return materialAsset.header.fileHeader.Matches(kCookedMaterialAssetMagic, kCookedMaterialAssetVersion) &&
		       materialAsset.header.textureReferenceVersion == kCookedTextureReferenceVersion;
	}
}