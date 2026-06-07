#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"

#include <memory>
#include <span>
#include <vector>

class MeshComponent;

namespace SceneMorphWeightApplicator
{
	void Apply(
	    std::span<const SceneMorphWeightSnapshot> morphWeights,
	    const std::vector<std::unique_ptr<MeshComponent>>& meshComponents);
}
