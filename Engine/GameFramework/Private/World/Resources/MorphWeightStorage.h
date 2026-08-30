#pragma once

#include "World/ECS/Components/AnimationComponents.h"

#include <span>
#include <vector>

namespace ECS
{
	class MorphWeightStorage final
	{
	public:
		AnimationOutputSlotHandle Add(std::span<const float> weights);
		bool PrepareWriteSize(AnimationOutputSlotHandle handle, std::size_t weightCount);
		bool Write(AnimationOutputSlotHandle handle, std::span<const float> weights) noexcept;
		std::span<const float> Read(AnimationOutputSlotHandle handle) const noexcept;
		bool Remove(AnimationOutputSlotHandle handle) noexcept;
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
