#pragma once

#include "GameFramework/Public/World/EntityId.h"

#include <vector>

namespace ECS
{
	class GameWorldState;

	class SystemChangeCommitter final
	{
	  public:
		static bool CommitSystemOutputs(GameWorldState& state, float nextMotionTime);
		static bool CommitExtraction(GameWorldState& state);

	  private:
		static void SortUnique(std::vector<EntityId>& entities);
		static void RecordTransformChanges(GameWorldState& state, std::vector<EntityId>& changes);
	};
}
