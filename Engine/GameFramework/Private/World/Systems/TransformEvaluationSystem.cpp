#include "PCH.h"
#include "World/Systems/TransformEvaluationSystem.h"

#include "World/ECS/Components/TransformComponents.h"
#include "World/WorldTransformConversion.h"

namespace ECS
{
	void TransformEvaluationSystem::Evaluate(const LocalTransform& local, WorldTransform& world) noexcept
	{
		world = WorldTransformConversion::BuildWorld(local);
	}
}
