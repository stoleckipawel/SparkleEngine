#include "PCH.h"

#include "Features/MaterialVariants/CookedSceneMaterialVariantBuilder.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cstddef>
#include <cstring>
#include <format>
#include <string_view>

class CookedMaterialVariantTranslation final
{
  public:
	static void CopyVariantName(std::string_view sourceName, char (&outName)[Assets::kCookedSceneMaterialVariantNameCapacity]) noexcept
	{
		std::memcpy(outName, sourceName.data(), sourceName.size());
	}

	static Assets::CookedSceneMaterialVariantRecord BuildVariantRecord(const ImportedMaterialVariant& importedVariant)
	{
		Assets::CookedSceneMaterialVariantRecord record;
		CopyVariantName(importedVariant.name, record.name);
		record.sourceVariantIndex = importedVariant.sourceVariantIndex;
		return record;
	}

	static std::uint32_t ResolveCookedMeshAssetIndex(
	    const SourceImportOutput& importOutput,
	    const ImportedMaterialVariantMapping& importedMapping)
	{
		for (std::size_t primitiveIndex = 0; primitiveIndex < importOutput.scene.meshPrimitives.size(); ++primitiveIndex)
		{
			const ImportedMeshPrimitive& primitive = importOutput.scene.meshPrimitives[primitiveIndex];
			if (primitive.sourceMeshIndex == importedMapping.sourceMeshIndex &&
			    primitive.sourcePrimitiveIndex == importedMapping.sourcePrimitiveIndex)
			{
				return static_cast<std::uint32_t>(primitiveIndex);
			}
		}

		throw Diagnostics::Error(std::format(
		    "Imported material variant mapping references source mesh {} primitive {}, but no cooked mesh asset was produced.",
		    importedMapping.sourceMeshIndex,
		    importedMapping.sourcePrimitiveIndex));
	}

	static Assets::CookedSceneMaterialVariantMappingRecord BuildMappingRecord(
	    const SourceImportOutput& importOutput,
	    const CookedSceneBuild& build,
	    const ImportedMaterialVariantMapping& importedMapping)
	{
		if (importedMapping.variantIndex >= build.manifest.materialVariants.size())
		{
			throw Diagnostics::Error(
			    "Imported material variant mapping references a variant outside the cooked variant set.");
		}

		if (importedMapping.materialIndex >= build.outputs.materialAssets.size())
		{
			throw Diagnostics::Error(
			    "Imported material variant mapping references a material outside the cooked material set.");
		}

		const std::uint32_t meshAssetIndex = ResolveCookedMeshAssetIndex(importOutput, importedMapping);

		if (meshAssetIndex >= build.manifest.meshAssetReferences.size())
		{
			throw Diagnostics::Error(
			    "Imported material variant mapping resolved to a mesh asset outside the cooked mesh asset set.");
		}

		return Assets::CookedSceneMaterialVariantMappingRecord{
		    .meshAssetIndex = meshAssetIndex,
		    .variantIndex = importedMapping.variantIndex,
		    .materialAssetIndex = importedMapping.materialIndex};
	}
};

void CookedSceneMaterialVariantBuilder::BuildMaterialVariants(
    const SourceImportOutput& importOutput,
    CookedSceneBuild& outBuild)
{
	outBuild.manifest.materialVariants.clear();
	outBuild.manifest.materialVariantMappings.clear();
	outBuild.manifest.materialVariants.reserve(importOutput.scene.materialVariants.size());
	outBuild.manifest.materialVariantMappings.reserve(importOutput.scene.materialVariantMappings.size());

	for (const ImportedMaterialVariant& importedVariant : importOutput.scene.materialVariants)
	{
		if (importedVariant.name.empty() || importedVariant.name.size() >= Assets::kCookedSceneMaterialVariantNameCapacity)
		{
			throw Diagnostics::Error("Imported material variant has an invalid name.");
		}
		outBuild.manifest.materialVariants.push_back(CookedMaterialVariantTranslation::BuildVariantRecord(importedVariant));
	}

	for (const ImportedMaterialVariantMapping& importedMapping : importOutput.scene.materialVariantMappings)
	{
		outBuild.manifest.materialVariantMappings.push_back(
		    CookedMaterialVariantTranslation::BuildMappingRecord(importOutput, outBuild, importedMapping));
	}
}
