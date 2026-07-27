#include "PCH.h"

#include "Features/MaterialVariants/CookedSceneMaterialVariantBuilder.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <format>
#include <string_view>

class CookedMaterialVariantTranslation final
{
  public:
	static void CopyVariantName(std::string_view sourceName, char (&outName)[Assets::kCookedSceneMaterialVariantNameCapacity]) noexcept
	{
		const std::size_t copyLength =
		    (std::min)(sourceName.size(), static_cast<std::size_t>(Assets::kCookedSceneMaterialVariantNameCapacity - 1u));
		if (copyLength > 0)
		{
			std::memcpy(outName, sourceName.data(), copyLength);
		}
	}

	static Assets::CookedSceneMaterialVariantRecord BuildVariantRecord(const ImportedMaterialVariant& importedVariant)
	{
		Assets::CookedSceneMaterialVariantRecord record;
		CopyVariantName(importedVariant.name, record.name);
		record.sourceVariantIndex = importedVariant.sourceVariantIndex;
		return record;
	}

	static bool ResolveCookedMeshAssetIndex(
	    const SourceImportResult& importResult,
	    const ImportedMaterialVariantMapping& importedMapping,
	    std::uint32_t& outMeshAssetIndex,
	    std::string& outErrorMessage)
	{
		for (std::size_t primitiveIndex = 0; primitiveIndex < importResult.scene.meshPrimitives.size(); ++primitiveIndex)
		{
			const ImportedMeshPrimitive& primitive = importResult.scene.meshPrimitives[primitiveIndex];
			if (primitive.sourceMeshIndex == importedMapping.sourceMeshIndex &&
			    primitive.sourcePrimitiveIndex == importedMapping.sourcePrimitiveIndex)
			{
				outMeshAssetIndex = static_cast<std::uint32_t>(primitiveIndex);
				return true;
			}
		}

		outErrorMessage = std::format(
		    "Imported material variant mapping references source mesh {} primitive {}, but no cooked mesh asset was produced",
		    importedMapping.sourceMeshIndex,
		    importedMapping.sourcePrimitiveIndex);
		return false;
	}

	static bool BuildMappingRecord(
	    const SourceImportResult& importResult,
	    const CookedSceneBuild& build,
	    const ImportedMaterialVariantMapping& importedMapping,
	    Assets::CookedSceneMaterialVariantMappingRecord& outRecord,
	    std::string& outErrorMessage)
	{
		if (importedMapping.variantIndex >= build.manifest.materialVariants.size())
		{
			outErrorMessage = "Imported material variant mapping references a variant outside the cooked variant set";
			return false;
		}

		if (importedMapping.materialIndex >= build.outputs.materialAssets.size())
		{
			outErrorMessage = "Imported material variant mapping references a material outside the cooked material set";
			return false;
		}

		std::uint32_t meshAssetIndex = 0;
		if (!ResolveCookedMeshAssetIndex(importResult, importedMapping, meshAssetIndex, outErrorMessage))
		{
			return false;
		}

		if (meshAssetIndex >= build.manifest.meshAssetReferences.size())
		{
			outErrorMessage = "Imported material variant mapping resolved to a mesh asset outside the cooked mesh asset set";
			return false;
		}

		outRecord.meshAssetIndex = meshAssetIndex;
		outRecord.variantIndex = importedMapping.variantIndex;
		outRecord.materialAssetIndex = importedMapping.materialIndex;
		return true;
	}
};

bool CookedSceneMaterialVariantBuilder::BuildMaterialVariants(
    const SourceImportResult& importResult,
    CookedSceneBuild& outBuild,
    std::string& outErrorMessage)
{
	outBuild.manifest.materialVariants.clear();
	outBuild.manifest.materialVariantMappings.clear();
	outBuild.manifest.materialVariants.reserve(importResult.scene.materialVariants.size());
	outBuild.manifest.materialVariantMappings.reserve(importResult.scene.materialVariantMappings.size());

	for (const ImportedMaterialVariant& importedVariant : importResult.scene.materialVariants)
	{
		outBuild.manifest.materialVariants.push_back(CookedMaterialVariantTranslation::BuildVariantRecord(importedVariant));
	}

	for (const ImportedMaterialVariantMapping& importedMapping : importResult.scene.materialVariantMappings)
	{
		Assets::CookedSceneMaterialVariantMappingRecord record;
		if (!CookedMaterialVariantTranslation::BuildMappingRecord(importResult, outBuild, importedMapping, record, outErrorMessage))
		{
			return false;
		}

		outBuild.manifest.materialVariantMappings.push_back(record);
	}

	outErrorMessage.clear();
	return true;
}
