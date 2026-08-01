#include "PCH.h"

#include "World/Systems/GameSystemGraph.h"

#include "World/Systems/CompiledGameSystemGraphData.h"

#include <memory>
#include <utility>

namespace ECS
{
	CompiledGameSystemGraph::CompiledGameSystemGraph() noexcept = default;
	CompiledGameSystemGraph::~CompiledGameSystemGraph() = default;
	CompiledGameSystemGraph::CompiledGameSystemGraph(CompiledGameSystemGraph&&) noexcept = default;
	CompiledGameSystemGraph& CompiledGameSystemGraph::operator=(CompiledGameSystemGraph&&) noexcept = default;
	CompiledGameSystemGraph::CompiledGameSystemGraph(std::unique_ptr<CompiledGameSystemGraphData> data) noexcept : m_data(std::move(data))
	{
	}

	bool CompiledGameSystemGraph::IsValid() const noexcept
	{
		return m_data != nullptr && !m_data->Error && m_data->Tasks.IsValid();
	}

	const GameSystemGraphError& CompiledGameSystemGraph::GetError() const noexcept
	{
		static const GameSystemGraphError invalid{GameSystemGraphErrorCode::TaskGraphRejected, "Game-system graph is empty."};
		return m_data != nullptr ? m_data->Error : invalid;
	}

	std::span<const GameSystemDesc> CompiledGameSystemGraph::GetSystems() const noexcept
	{
		return m_data != nullptr ? std::span<const GameSystemDesc>(m_data->Systems) : std::span<const GameSystemDesc>{};
	}

	void GameSystemGraph::Add(GameSystemDesc descriptor)
	{
		m_systems.push_back(std::move(descriptor));
	}
}
