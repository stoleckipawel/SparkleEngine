#pragma once

#include "World/Systems/Descriptors/GameWorldSystemContract.h"

namespace ECS
{
	class GameWorldState;

	class TransformSystemExecution final
	{
	public:
		TransformSystemExecution(GameWorldState& state, const StructureFrozenEpoch& epoch);

		std::uint32_t GetDirtyTransformCount() const noexcept;
		bool RunTransforms(std::uint32_t begin, std::uint32_t end);
		bool RunCameraDerivedState(std::uint32_t begin, std::uint32_t end);

	private:
		GameWorldState& m_state;
		TransformEvaluationQuery m_transformQuery;
		CameraDerivedStateQuery m_cameraDerivedQuery;
	};
}
