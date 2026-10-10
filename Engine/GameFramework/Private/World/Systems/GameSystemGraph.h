#pragma once

#include "Tasks/Public/ParallelFor.h"
#include "World/ECS/QueryAccess.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class CompiledTaskGraph;
class TaskExecutor;

namespace ECS
{
	struct CompiledGameSystemGraphData;

	class GameWorldSystemExecution;

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
		CameraNavigationSettings,
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

	struct GameSystemDesc final
	{
		std::string Name;
		GameSystemPhase Phase = GameSystemPhase::Simulation;
		std::vector<ComponentAccessDesc> Components;
		std::vector<GameSystemResourceAccess> Resources;
		std::vector<std::uint32_t> Prerequisites;
		ParallelForPolicy RangePolicy;
		std::uint32_t (*GetItemCount)(GameWorldSystemExecution&) = nullptr;
		bool (*ExecuteRange)(GameWorldSystemExecution&, std::uint32_t begin, std::uint32_t end) = nullptr;

		template <typename QueryType> void DeclareQuery()
		{
			const auto access = QueryType::GetAccessMetadata();
			Components.assign(access.begin(), access.end());
		}
	};

	enum class GameSystemGraphErrorCode : std::uint8_t
	{
		None,
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
		ExecutionFailed,
	};

	struct GameSystemGraphError final
	{
		GameSystemGraphErrorCode Code = GameSystemGraphErrorCode::None;
		std::string Message;

		explicit operator bool() const noexcept { return Code != GameSystemGraphErrorCode::None; }
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
		bool Execute(TaskExecutor& executor, GameWorldSystemExecution& systems, GameSystemGraphError& error);

	private:
		friend class GameSystemGraph;
		explicit CompiledGameSystemGraph(std::unique_ptr<CompiledGameSystemGraphData> data) noexcept;
		std::unique_ptr<CompiledGameSystemGraphData> m_data;
	};

	class GameSystemGraph final
	{
	public:
		std::uint32_t Add(GameSystemDesc descriptor);
		CompiledGameSystemGraph Compile();

	private:
		std::vector<GameSystemDesc> m_systems;
	};
}
