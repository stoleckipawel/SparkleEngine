#pragma once

#include "World/ECS/Components/TransformComponents.h"

class Transform;

namespace ECS::WorldTransformConversion
{
	LocalTransform ToLocal(const Transform& transform) noexcept;
	Transform ToPublic(const LocalTransform& transform) noexcept;
	WorldTransform BuildWorld(const LocalTransform& transform) noexcept;
}
