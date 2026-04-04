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
	static void NormalizeTransformCount(SceneImportResult& result);
	static void NormalizeMaterialHandleCount(SceneImportResult& result);
	static void NormalizeMaterialTextures(SceneImportResult& result);
	static void SanitizeMaterialHandles(SceneImportResult& result);
	static void NormalizeOptionalTexturePath(
	    std::optional<std::filesystem::path>& texturePath,
	    std::string_view materialName,
	    std::string_view slotName,
	    SceneImportResult& result);
};