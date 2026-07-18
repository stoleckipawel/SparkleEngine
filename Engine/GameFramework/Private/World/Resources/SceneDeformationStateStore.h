#pragma once

#include "World/ECS/Components/AnimationComponents.h"

#include <span>
#include <vector>

namespace ECS
{
	class SceneDeformationStateStore final
	{
	  public:
		SceneStateHandle AddMorphWeights(std::span<const float> weights);
		bool WriteMorphWeights(SceneStateHandle handle, std::span<const float> weights);
		std::span<const float> ReadMorphWeights(SceneStateHandle handle) const noexcept;
		bool Remove(SceneStateHandle handle) noexcept;
		void Clear() noexcept;

	  private:
		struct Entry final
		{
			std::vector<float> Weights;
			std::uint32_t Generation = 1;
			bool Occupied = false;
		};

		std::vector<Entry> m_entries;
		std::vector<std::uint32_t> m_freeSlots;
	};
}
