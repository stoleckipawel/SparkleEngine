#include "PCH.h"

#include "Assets/Loaders/MaterialAssetLoader.h"

#include "Assets/Cooked/LoadedMaterialAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include <utility>

namespace Assets
{
	LoadedMaterialAsset MaterialAssetLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes) const
	{
		const CookedAssetLoaderDiagnostics diagnostics(path, "CookedMaterialAsset");

		CookedAssetByteReader reader(bytes);
		LoadedMaterialAsset materialAsset;
		materialAsset.header = reader.Read<CookedMaterialAssetHeader>();

		if (!materialAsset.header.fileHeader.HasMagic(kCookedMaterialAssetMagic))
		{
			throw diagnostics.MakeError(
			    "header",
			    "material magic",
			    "Invalid cooked material asset header");
		}

		materialAsset.name = reader.ReadString(materialAsset.header.nameByteCount);
		const std::vector<CookedTextureReferenceRecord> textureReferenceRecords =
		    reader.ReadArray<CookedTextureReferenceRecord>(materialAsset.header.textureReferenceCount);

		materialAsset.textureReferences.reserve(textureReferenceRecords.size());
		for (const CookedTextureReferenceRecord& textureReferenceRecord : textureReferenceRecords)
		{
			materialAsset.textureReferences.push_back(
			    CookedTextureReference{
			        .texturePath = reader.ReadString(textureReferenceRecord.texturePathByteCount),
			        .textureGroup = textureReferenceRecord.textureGroup});
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			throw diagnostics.MakeError(
			    "payload",
			    "no trailing bytes after declared material records",
			    "Cooked material asset contains unexpected trailing bytes");
		}

		return materialAsset;
	}
}
