#include "PCH.h"

#include "Assets/Loaders/MaterialAssetLoader.h"

#include "Assets/Cooked/LoadedMaterialAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include <utility>

namespace Assets
{
	bool MaterialAssetLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes,
	    LoadedMaterialAsset& outMaterialAsset,
	    std::string& outErrorMessage) const
	{
		const CookedAssetLoaderContext diagnosticsContext =
		    CookedAssetLoaderDiagnostics::BuildContext(path, "CookedMaterialAsset", kCookedMaterialAssetVersion);
		auto fail = [&](std::string_view recordKind, std::string_view expectedFeature, std::string_view reason) -> bool
		{
			CookedAssetLoaderDiagnostics::SetFailure(diagnosticsContext, recordKind, expectedFeature, reason, outErrorMessage);
			return false;
		};

		CookedAssetByteReader reader(bytes);
		if (!reader.Read(outMaterialAsset.header, outErrorMessage))
		{
			return fail("header", "CookedMaterialAssetHeader", outErrorMessage);
		}

		if (!HasValidHeader(outMaterialAsset))
		{
			return fail("header", "material magic/version and texture reference version", "Invalid cooked material asset header");
		}

		std::vector<CookedTextureReferenceRecord> textureReferenceRecords;
		if (!reader.ReadString(outMaterialAsset.header.nameByteCount, outMaterialAsset.name, outErrorMessage) ||
		    !reader.ReadArray(outMaterialAsset.header.textureReferenceCount, textureReferenceRecords, outErrorMessage))
		{
			return fail("payload", "material name and texture reference records matching header counts", outErrorMessage);
		}

		outMaterialAsset.textureReferences.clear();
		outMaterialAsset.textureReferences.reserve(textureReferenceRecords.size());
		for (const CookedTextureReferenceRecord& textureReferenceRecord : textureReferenceRecords)
		{
			CookedTextureReference textureReference;
			textureReference.textureGroup = textureReferenceRecord.textureGroup;
			if (!reader.ReadString(textureReferenceRecord.texturePathByteCount, textureReference.texturePath, outErrorMessage))
			{
				return fail("textureReference.path", "texture path string matching reference byte count", outErrorMessage);
			}

			outMaterialAsset.textureReferences.push_back(std::move(textureReference));
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			return fail("payload", "no trailing bytes after declared material records", "Cooked material asset contains unexpected trailing bytes");
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
