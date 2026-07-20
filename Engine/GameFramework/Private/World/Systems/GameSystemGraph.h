#pragma once

#include "Tasks/Public/ParallelFor.h"
#include "World/ECS/QueryAccess.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class CompiledTaskGraph;
class TaskExecutor;

namespace ECS
{
	struct GameSystemId final
	{
		std::uint64_t Value = 0;

		constexpr bool IsValid() const noexcept { return Value != 0; }
		constexpr auto operator<=>(const GameSystemId&) const noexcept = default;
	};

	constexpr GameSystemId MakeGameSystemId(std::string_view canonicalName) noexcept
	{
		constexpr std::uint64_t OffsetBasis = 14695981039346656037ull;
		constexpr std::uint64_t Prime = 1099511628211ull;
		std::uint64_t hash = OffsetBasis;
		for (char character : canonicalName)
		{
			hash ^= static_cast<std::uint8_t>(character);
			hash *= Prime;
		}
		return GameSystemId{hash};
	}

	enum class GameSystemPhase : std::uint8_t
	{
		Simulation,
		Animation,
		Deformation,
		Transform,
		Extraction,
	};

	enum class GameSystemResourceDomain : std::uint8_t
	{
		UpdateInputs,
		CameraInputIntent,
		MotionClock,
		SystemChangeScratch,
		AnimationClips,
		SkeletonResources,
		PoseScratch,
		MorphScratch,
		SkinningOutput,
		MorphOutput,
		DirtyTransforms,
		TransformScratch,
		CameraDerivedScratch,
		WorldChanges,
		MeshResources,
		ExtractionScratch,
		ExtractionOutput,
		WorldPublication,
	};

	enum class GameSystemAccessMode : std::uint8_t
	{
		Read,
		Write,
	};

	struct GameSystemResourceAccess final
	{
		GameSystemResourceDomain Domain = GameSystemResourceDomain::UpdateInputs;
		GameSystemAccessMode Mode = GameSystemAccessMode::Read;
	};

	enum class GameSystemExecutionMode : std::uint8_t
	{
		SingleTask,
		ParallelRanges,
	};

	struct GameSystemExecutionPolicy final
	{
		GameSystemExecutionMode Mode = GameSystemExecutionMode::SingleTask;
		ParallelForPolicy RangePolicy{};
	};

	struct GameSystemDesc final
	{
		GameSystemId Id;
		std::string Name;
		GameSystemPhase Phase = GameSystemPhase::Simulation;
		std::vector<ComponentAccessDesc> Components;
		std::vector<GameSystemResourceAccess> Resources;
		std::vector<GameSystemId> Prerequisites;
		GameSystemExecutionPolicy Execution;

		template <typename QueryType> void DeclareQuery()
		{
			const auto access = QueryType::GetAccessMetadata();
			Components.assign(access.begin(), access.end());
		}
	};

	enum class GameSystemGraphErrorCode : std::uint8_t
	{
		None,
		EmptySystemId,
		EmptySystemName,
		DuplicateSystem,
		MissingPrerequisite,
		InvalidPhaseDependency,
		DuplicateAccess,
		ConflictingAccessDeclaration,
		UndeclaredAccess,
		UnavailablePhaseResource,
		AmbiguousHazard,
		Cycle,
		TaskGraphRejected,
		BindingMismatch,
		ExecutionFailed,
	};

	struct GameSystemGraphError final
	{
		GameSystemGraphErrorCode Code = GameSystemGraphErrorCode::None;
		std::string Message;

		explicit operator bool() const noexcept { return Code != GameSystemGraphErrorCode::None; }
	};

	using GameSystemItemCountFunction = std::function<std::uint32_t()>;
	using GameSystemRangeFunction = std::function<bool(std::uint32_t begin, std::uint32_t end)>;

	struct GameSystemExecutionBinding final
	{
		GameSystemId Id;
		GameSystemItemCountFunction GetItemCount;
		GameSystemRangeFunction ExecuteRange;
	};

	class CompiledGameSystemGraph final
	{
	  public:
		CompiledGameSystemGraph() noexcept;
		~CompiledGameSystemGraph();
		CompiledGameSystemGraph(CompiledGameSystemGraph&&) noexcept;
		CompiledGameSystemGraph& operator=(CompiledGameSystemGraph&&) noexcept;
		CompiledGameSystemGraph(const CompiledGameSystemGraph&) = delete;
		CompiledGameSystemGraph& operator=(const CompiledGameSystemGraph&) = delete;

		bool IsValid() const noexcept;
		explicit operator bool() const noexcept { return IsValid(); }
		const GameSystemGraphError& GetError() const noexcept;
		std::span<const GameSystemDesc> GetSystems() const noexcept;
		bool Execute(TaskExecutor& executor, std::span<const GameSystemExecutionBinding> bindings, GameSystemGraphError& error) const;

	  private:
		friend class GameSystemGraph;
		struct Data;
		explicit CompiledGameSystemGraph(std::unique_ptr<Data> data) noexcept;
		std::unique_ptr<Data> m_data;
	};

	class GameSystemGraph final
	{
	  public:
		void Add(GameSystemDesc descriptor);
		CompiledGameSystemGraph Compile() const;

	  private:
		std::vector<GameSystemDesc> m_systems;
	};
}
