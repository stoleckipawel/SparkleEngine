#pragma once

#include "SourceImportOutput.h"

#include <assimp/scene.h>

class FbxCameraImporter final
{
  public:
	static void ImportCameras(const aiScene& scene, SourceImportOutput& output);

  private:
	static float ResolveFovYRadians(const aiCamera& camera) noexcept;
};
