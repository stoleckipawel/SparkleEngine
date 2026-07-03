#include "PCH.h"
#include "FrameGraph/Diagnostics/FrameGraphPlanDiagnostics.h"

#include "FrameGraph/Compiler/FrameGraphPlan.h"

#include "Core/Public/Environment/EnvironmentVariables.h"

#include <algorithm>
#include <string>
#include <string_view>

namespace FrameGraphPlanDiagnostics
{
	struct Config
	{
		bool Enabled = false;
		std::string Filter;
	};

	static Config LoadConfig() noexcept
	{
		Config config{};
		config.Enabled = Environment::GetFlag("SPARKLE_FRAMEGRAPH_DIAGNOSTICS");
		if (config.Enabled)
		{
			Environment::TryGetVariable("SPARKLE_FRAMEGRAPH_DIAGNOSTICS_FILTER", config.Filter);
		}

		return config;
	}

	static bool Contains(std::string_view value, std::string_view filter) noexcept
	{
		return filter.empty() || value.find(filter) != std::string_view::npos;
	}

	static bool MatchesPassFilter(const FrameGraphPassNode& passRecord, std::string_view filter) noexcept
	{
		return Contains(passRecord.passName, filter) || Contains(passRecord.diagnosticName, filter) ||
		       Contains(passRecord.displayLabel, filter);
	}

	static bool MatchesResourceFilter(const FrameGraphResourceNode& resourceRecord, std::string_view filter) noexcept
	{
		return Contains(resourceRecord.debugName, filter);
	}

	static const FrameGraphPassNode* FindPass(const FrameGraphPlan& plan, FrameGraphPassIndex passIndex) noexcept
	{
		return passIndex < plan.passes.size() ? &plan.passes[passIndex] : nullptr;
	}

	static const FrameGraphResourceNode* FindResource(const FrameGraphPlan& plan, FrameGraphResourceHandle handle) noexcept
	{
		const auto it = std::find_if(
		    plan.resources.begin(),
		    plan.resources.end(),
		    [handle](const FrameGraphResourceNode& resourceRecord) noexcept
		    {
			    return resourceRecord.handle == handle;
		    });

		return it != plan.resources.end() ? &*it : nullptr;
	}

	static std::string_view ResourceName(const FrameGraphResourceNode* resourceRecord) noexcept
	{
		if (resourceRecord == nullptr)
		{
			return "<unknown>";
		}

		return resourceRecord->debugName.empty() ? "<unnamed>" : std::string_view(resourceRecord->debugName);
	}

	static const char* BarrierTypeName(FrameGraphBarrier::Type type) noexcept
	{
		switch (type)
		{
			case FrameGraphBarrier::Type::Transition:
				return "Transition";
			case FrameGraphBarrier::Type::UnorderedAccess:
				return "UnorderedAccess";
			case FrameGraphBarrier::Type::AccelerationStructure:
				return "AccelerationStructure";
			default:
				return "Unknown";
		}
	}

	static const char* ResourceKindName(FrameGraphResourceKind kind) noexcept
	{
		switch (kind)
		{
			case FrameGraphResourceKind::BackBuffer:
				return "BackBuffer";
			case FrameGraphResourceKind::DepthStencil:
				return "DepthStencil";
			case FrameGraphResourceKind::ColorRenderTarget:
				return "ColorRenderTarget";
			case FrameGraphResourceKind::Buffer:
				return "Buffer";
			case FrameGraphResourceKind::AccelerationStructure:
				return "AccelerationStructure";
			default:
				return "Unknown";
		}
	}

	static const char* ResourceOwnershipName(FrameGraphResourceOwnership ownership) noexcept
	{
		switch (ownership)
		{
			case FrameGraphResourceOwnership::Transient:
				return "Transient";
			case FrameGraphResourceOwnership::Imported:
				return "Imported";
			case FrameGraphResourceOwnership::ExternalPersistent:
				return "ExternalPersistent";
			default:
				return "Unknown";
		}
	}

	static std::string_view BarrierLabel(const FrameGraphBarrier& barrier) noexcept
	{
		return barrier.label.empty() ? "<unlabeled>" : std::string_view(barrier.label);
	}

	static const char* AllocationPoolName(FrameGraphTransientResourcePlan::AllocationPool pool) noexcept
	{
		switch (pool)
		{
			case FrameGraphTransientResourcePlan::AllocationPool::Color:
				return "Color";
			case FrameGraphTransientResourcePlan::AllocationPool::Depth:
				return "Depth";
			case FrameGraphTransientResourcePlan::AllocationPool::Buffer:
				return "Buffer";
			default:
				return "Unknown";
		}
	}

	static std::string_view PassNameForIndex(const FrameGraphPlan& plan, FrameGraphPassIndex passIndex) noexcept
	{
		const FrameGraphPassNode* passRecord = FindPass(plan, passIndex);
		return passRecord != nullptr ? std::string_view(passRecord->passName) : "<invalid>";
	}

	static void LogPassDeclarations(const FrameGraphPlan& plan, const Config& config, const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		for (const FrameGraphPassNode& passRecord : plan.passes)
		{
			for (const PassResourceDeclaration& declaration : passRecord.declarations)
			{
				const FrameGraphResourceNode* resourceRecord = FindResource(plan, declaration.handle);
				if (!MatchesPassFilter(passRecord, config.Filter) &&
				    (resourceRecord == nullptr || !MatchesResourceFilter(*resourceRecord, config.Filter)))
				{
					continue;
				}

			}
		}
	}

	static bool ShouldLogBarrier(
	    const FrameGraphPlan& plan,
	    const FrameGraphPassNode* passRecord,
	    const FrameGraphBarrier& barrier,
	    std::string_view filter) noexcept
	{
		if (filter.empty() || (passRecord != nullptr && MatchesPassFilter(*passRecord, filter)))
		{
			return true;
		}

		const FrameGraphResourceNode* resourceRecord = FindResource(plan, barrier.handle);
		return resourceRecord != nullptr && MatchesResourceFilter(*resourceRecord, filter);
	}

	static bool ShouldLogAliasingBarrier(
	    const FrameGraphPlan& plan,
	    const FrameGraphPassNode* passRecord,
	    const FrameGraphAliasingBarrier& barrier,
	    std::string_view filter) noexcept
	{
		if (filter.empty() || (passRecord != nullptr && MatchesPassFilter(*passRecord, filter)))
		{
			return true;
		}

		const FrameGraphResourceNode* beforeResource = FindResource(plan, barrier.beforeHandle);
		const FrameGraphResourceNode* afterResource = FindResource(plan, barrier.afterHandle);
		return (beforeResource != nullptr && MatchesResourceFilter(*beforeResource, filter)) ||
		       (afterResource != nullptr && MatchesResourceFilter(*afterResource, filter));
	}

	static void LogPassOrder(const FrameGraphPlan& plan, const Config& config, const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		for (std::size_t orderIndex = 0; orderIndex < plan.executionOrder.size(); ++orderIndex)
		{
			const FrameGraphPassNode* passRecord = FindPass(plan, plan.executionOrder[orderIndex]);
			if (passRecord == nullptr || !MatchesPassFilter(*passRecord, config.Filter))
			{
				continue;
			}

		}
	}

	static void LogCulledPasses(const FrameGraphPlan& plan, const Config& config, const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		for (const FrameGraphPassNode& passRecord : plan.passes)
		{
			if (passRecord.alive || !MatchesPassFilter(passRecord, config.Filter))
			{
				continue;
			}

		}
	}

	static void LogResourceLifetimes(const FrameGraphPlan& plan, const Config& config, const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		for (const FrameGraphResourceNode& resourceRecord : plan.resources)
		{
			if (!MatchesResourceFilter(resourceRecord, config.Filter))
			{
				continue;
			}

		}

		for (const FrameGraphTransientResourcePlan& transientPlan : plan.transients.resources)
		{
			const FrameGraphResourceNode* resourceRecord = FindResource(plan, transientPlan.handle);
			if (resourceRecord == nullptr || !MatchesResourceFilter(*resourceRecord, config.Filter))
			{
				continue;
			}

		}
	}

	static void LogBarriers(const FrameGraphPlan& plan, const Config& config, const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		for (const FrameGraphPassNode& passRecord : plan.passes)
		{
			for (const FrameGraphBarrier& barrier : passRecord.compiledBarriers)
			{
				if (!ShouldLogBarrier(plan, &passRecord, barrier, config.Filter))
				{
					continue;
				}

				const FrameGraphResourceNode* resourceRecord = FindResource(plan, barrier.handle);
			}
		}

		for (const FrameGraphBarrier& barrier : plan.finalBarriers)
		{
			if (!ShouldLogBarrier(plan, nullptr, barrier, config.Filter))
			{
				continue;
			}

			const FrameGraphResourceNode* resourceRecord = FindResource(plan, barrier.handle);
		}
	}

	static void LogAliasingBlocks(const FrameGraphPlan& plan, const Config& config, const std::shared_ptr<spdlog::logger>& logger) noexcept
	{
		for (const FrameGraphTransientPhysicalBlockPlan& blockPlan : plan.transients.physicalBlocks)
		{
			if (!config.Filter.empty())
			{
				const bool blockMatches = std::any_of(
				    blockPlan.handles.begin(),
				    blockPlan.handles.end(),
				    [&plan, &config](FrameGraphResourceHandle handle) noexcept
				    {
					    const FrameGraphResourceNode* resourceRecord = FindResource(plan, handle);
					    return resourceRecord != nullptr && MatchesResourceFilter(*resourceRecord, config.Filter);
				    });

				if (!blockMatches)
				{
					continue;
				}
			}

		}

		for (const FrameGraphPassNode& passRecord : plan.passes)
		{
			for (const FrameGraphAliasingBarrier& barrier : passRecord.transientAliasingBarriers)
			{
				if (!ShouldLogAliasingBarrier(plan, &passRecord, barrier, config.Filter))
				{
					continue;
				}

			}
		}

		for (const FrameGraphAliasingBarrier& barrier : plan.finalTransientAliasingBarriers)
		{
			if (!ShouldLogAliasingBarrier(plan, nullptr, barrier, config.Filter))
			{
				continue;
			}

		}
	}

	void LogIfEnabled(const FrameGraphPlan& plan) noexcept
	{
		const Config config = LoadConfig();
		if (!config.Enabled)
		{
			return;
		}

		const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.FrameGraph");
		const std::size_t alivePassCount = static_cast<std::size_t>(std::count_if(
		    plan.passes.begin(),
		    plan.passes.end(),
		    [](const FrameGraphPassNode& passRecord) noexcept
		    {
			    return passRecord.alive;
		    }));


		LogPassOrder(plan, config, logger);
		LogPassDeclarations(plan, config, logger);
		LogCulledPasses(plan, config, logger);
		LogResourceLifetimes(plan, config, logger);
		LogBarriers(plan, config, logger);
		LogAliasingBlocks(plan, config, logger);
	}
}
