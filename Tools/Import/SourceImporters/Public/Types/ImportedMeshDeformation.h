#pragma once

#include "ImportedMorphTarget.h"
#include "ImportedSkin.h"

#include <vector>

struct ImportedMeshDeformation
{
	std::vector<ImportedSkinInfluence> skinInfluences;
	std::vector<ImportedMorphTarget> morphTargets;

	bool HasSkinInfluences() const noexcept { return !skinInfluences.empty(); }
	bool HasMorphTargets() const noexcept { return !morphTargets.empty(); }
};
