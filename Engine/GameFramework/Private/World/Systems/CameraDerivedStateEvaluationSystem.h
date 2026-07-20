#pragma once

namespace ECS
{
	struct LocalTransform;
	struct CameraDerivedState;

	class CameraDerivedStateEvaluationSystem final
	{
	  public:
		static void Evaluate(const LocalTransform& local, CameraDerivedState& derived) noexcept;
	};
}
