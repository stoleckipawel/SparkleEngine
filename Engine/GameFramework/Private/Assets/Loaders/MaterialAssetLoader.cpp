#include "PCH.h"

#include "Assets/Loaders/MaterialAssetLoader.h"

#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/LoadedCookedAssets.h"
#include "Core/Public/Files/FileUtils.h"

#include <utility>

namespace Assets
{
	bool MaterialAssetLoader::Load(const std::filesystem::path& path, LoadedMaterialAsset& outMaterialAsset, std::string& outErrorMessage)
	    const
	{
		std::vector<std::uint8_t> fileBytes;
		if (!Files::TryReadAllBytes(path, fileBytes, outErrorMessage))
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

		std::vector<CookedTextureReferenceRecord> textureReferenceRecords;
		if (!reader.ReadString(outMaterialAsset.header.nameByteCount, outMaterialAsset.name, outErrorMessage) ||
		    !reader.ReadArray(outMaterialAsset.header.textureReferenceCount, textureReferenceRecords, outErrorMessage))
		{
			return false;
		}

		outMaterialAsset.textureReferences.clear();
		outMaterialAsset.textureReferences.reserve(textureReferenceRecords.size());
		for (const CookedTextureReferenceRecord& textureReferenceRecord : textureReferenceRecords)
		{
			CookedTextureReference textureReference;
			textureReference.textureGroup = textureReferenceRecord.textureGroup;
			if (!reader.ReadString(textureReferenceRecord.texturePathByteCount, textureReference.texturePath, outErrorMessage))
			{
				return false;
			}

			outMaterialAsset.textureReferences.push_back(std::move(textureReference));
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