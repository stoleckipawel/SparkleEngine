#pragma once

#include "Memory/RhiMemoryDiagnostics.h"

#include <cstdint>
#include <algorithm>
#include <vector>

template <typename BlockIdentity> struct RhiMemoryCategoryAggregation final
{
	RhiMemoryCategoryStats Stats;
	std::vector<BlockIdentity> UniqueBlocks;
};

class RhiMemoryCategoryAggregationPolicy final
{
public:
	template <typename BlockIdentity> static RhiMemoryCategoryAggregation<BlockIdentity>& FindOrCreate(
	    std::vector<RhiMemoryCategoryAggregation<BlockIdentity>>& aggregations,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::uint64_t budgetBytes = 0)
	{
		const auto existing = std::find_if(
		    aggregations.begin(),
		    aggregations.end(),
		    [category, residencyClass](const RhiMemoryCategoryAggregation<BlockIdentity>& aggregation)
		    { return aggregation.Stats.Category == category && aggregation.Stats.ResidencyClass == residencyClass; });
		if (existing != aggregations.end())
		{
			return *existing;
		}

		aggregations.push_back(
		    RhiMemoryCategoryAggregation<BlockIdentity>{
		        .Stats = RhiMemoryCategoryStats{.Category = category, .ResidencyClass = residencyClass, .BudgetBytes = budgetBytes}});
		return aggregations.back();
	}

	template <typename BlockIdentity>
	static void AddUniqueBlock(RhiMemoryCategoryAggregation<BlockIdentity>& aggregation, BlockIdentity block, std::uint64_t budgetBytes = 0)
	{
		if (std::find(aggregation.UniqueBlocks.begin(), aggregation.UniqueBlocks.end(), block) != aggregation.UniqueBlocks.end())
		{
			return;
		}

		aggregation.UniqueBlocks.push_back(block);
		++aggregation.Stats.BlockCount;
		aggregation.Stats.BudgetBytes += budgetBytes;
	}
};
