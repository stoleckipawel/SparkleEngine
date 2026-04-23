#pragma once

#include "Cooking/Graph/CookNode.h"

#include <span>
#include <vector>

// Phase 1 placeholder: nodes carry no edges yet, so traversal order is
// insertion order. The class exists as the parallelism contract for later
// phases; today it just lets ShaderPackageCooker iterate uniformly via the
// executor seam (see docs/plans/shadercompiler-architecture-review.md §13).
class DependencyGraph final
{
  public:
	void AddNode(CookNode node);
	std::span<const CookNode> GetTopologicalOrder() const noexcept;
	std::size_t Size() const noexcept { return m_nodes.size(); }

  private:
	std::vector<CookNode> m_nodes;
};
