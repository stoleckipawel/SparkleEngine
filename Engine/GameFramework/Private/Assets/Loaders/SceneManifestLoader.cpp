#include "PCH.h"

#include "Assets/Loaders/SceneManifestLoader.h"

#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include "Assets/Loaders/SceneManifestValidator.h"
#include <cstdint>

namespace Assets
{
	bool SceneManifestLoader::Decode(
	    const std::filesystem::path& path,
	    std::span<const std::uint8_t> bytes,
	    LoadedSceneManifest& outManifest,
	    std::string& outErrorMessage) const
	{
		const CookedAssetLoaderContext diagnosticsContext =
		    CookedAssetLoaderDiagnostics::BuildContext(path, "CookedSceneManifest", kCookedSceneManifestVersion);
		auto fail = [&](std::string_view recordKind, std::string_view expectedFeature, std::string_view reason) -> bool
		{
			CookedAssetLoaderDiagnostics::SetFailure(diagnosticsContext, recordKind, expectedFeature, reason, outErrorMessage);
			return false;
		};

		CookedAssetByteReader reader(bytes);
		if (!reader.Read(outManifest.header, outErrorMessage))
		{
			return fail("header", "CookedSceneManifestHeader", outErrorMessage);
		}

		if (!SceneManifestValidator::ValidateHeader(outManifest, outErrorMessage))
		{
			return fail("header", "scene manifest magic/version/record counts/features", outErrorMessage);
		}

		if (!reader.ReadArray(outManifest.header.meshAssetReferenceCount, outManifest.meshAssetReferences, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.materialAssetReferenceCount, outManifest.materialAssetReferences, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.instanceCount, outManifest.instances, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.instanceGroupCount, outManifest.instanceGroups, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.cameraCount, outManifest.cameras, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.lightCount, outManifest.lights, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.skeletonRefCount, outManifest.skeletonRefs, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.animationRefCount, outManifest.animationReferences, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.morphWeightCount, outManifest.morphWeights, outErrorMessage) ||
		    !reader.ReadArray(outManifest.header.materialVariantCount, outManifest.materialVariants, outErrorMessage) ||
		    !reader.ReadArray(
		        outManifest.header.materialVariantMappingCount,
		        outManifest.materialVariantMappings,
		        outErrorMessage))
		{
			return fail("records", "all scene manifest arrays matching header counts", outErrorMessage);
		}

		if (!SceneManifestValidator::ValidateRecords(outManifest, outErrorMessage))
		{
			return fail("records", "scene manifest references resolve within declared arrays", outErrorMessage);
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			return fail("payload", "no trailing bytes after declared scene manifest records", "Cooked scene manifest contains unexpected trailing bytes");
		}

		outErrorMessage.clear();
		return true;
	}
}
