#include "PCH.h"

#include "Cooking/Graph/DependencyGraph.h"

void DependencyGraph::AddNode(CookNode node)
{
	m_nodes.push_back(std::move(node));
}

std::span<const CookNode> DependencyGraph::GetTopologicalOrder() const noexcept
{
	return m_nodes;
}
