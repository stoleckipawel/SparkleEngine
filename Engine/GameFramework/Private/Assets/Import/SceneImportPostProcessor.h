#pragma once

#include "Assets/Import/SceneImportResult.h"

#include <optional>
#include <string_view>

class SceneImportPostProcessor final
{
  public:
	static void Finalize(SceneImportResult& result);

	SceneImportPostProcessor() = delete;
	~SceneImportPostProcessor() = delete;

  private:
	static void NormalizeResultShape(SceneImportResult& result);
	static void NormalizeMaterialNamesAndTextures(SceneImportResult& result);
	static void DeduplicateMaterials(SceneImportResult& result);
	static void SanitizeMaterialOffsets(SceneImportResult& result);
	static void AccumulateStats(SceneImportResult& result);
	static bool AreMaterialsEquivalent(const MaterialDesc& lhs, const MaterialDesc& rhs) noexcept;
	static void NormalizeOptionalTexturePath(
	    std::optional<std::filesystem::path>& texturePath,
	    std::string_view importerName,
	    std::string_view materialName,
	    std::string_view slotName,
	    SceneImportResult& result);
};