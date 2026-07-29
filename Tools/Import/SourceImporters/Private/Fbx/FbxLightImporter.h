#pragma once

#include "SourceImportOutput.h"

#include <assimp/scene.h>

class FbxLightImporter final
{
  public:
	static void ImportLights(const aiScene& scene, SourceImportOutput& output);
};
