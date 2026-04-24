#pragma once

#include "Cooking/Graph/CookNode.h"

#include <span>
#include <vector>

// Phase 1 placeholder graph.
// With no edges yet, traversal order is just insertion order.
class DependencyGraph final
{
  public:
	void AddNode(CookNode node);
	std::span<const CookNode> GetTopologicalOrder() const noexcept;
	std::size_t Size() const noexcept { return m_nodes.size(); }

  private:
	std::vector<CookNode> m_nodes;
};
