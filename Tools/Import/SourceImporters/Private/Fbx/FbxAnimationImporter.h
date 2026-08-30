#pragma once

#include "SourceImportOutput.h"

#include <assimp/scene.h>

class FbxAnimationImporter final
{
public:
	static void ImportAnimations(const aiScene& scene, SourceImportOutput& output);
};
