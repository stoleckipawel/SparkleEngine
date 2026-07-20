#pragma once

namespace ECS
{
	struct LocalTransform;
	struct WorldTransform;

	class TransformEvaluationSystem final
	{
	  public:
		static void Evaluate(const LocalTransform& local, WorldTransform& world) noexcept;
	};
}
