#pragma once

#include "World/Systems/Descriptors/GameWorldSystemContract.h"

class SkeletonResourceStore;

namespace ECS
{
	class GameWorldState;

	class MeshExtractionSystemExecution final
	{
	  public:
		MeshExtractionSystemExecution(
		    GameWorldState& state,
		    const SkeletonResourceStore& skeletons,
		    const StructureFrozenEpoch& epoch);

		std::uint32_t GetMeshCount() const noexcept;
		bool Run(std::uint32_t begin, std::uint32_t end);

	  private:
		GameWorldState& m_state;
		const SkeletonResourceStore& m_skeletons;
		MeshExtractionQuery m_query;
	};
}
