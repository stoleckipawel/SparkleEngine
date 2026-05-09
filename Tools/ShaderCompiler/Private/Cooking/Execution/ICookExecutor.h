#pragma once

#include "Cooking/Graph/CookNode.h"
#include "Cooking/Graph/DependencyGraph.h"

#include <functional>
#include <string>

typedef std::function<bool(const CookNode& node, std::string& outErrorMessage)> CookNodeVisitor;

class ICookExecutor
{
  public:
	virtual ~ICookExecutor() = default;
	virtual bool Execute(const DependencyGraph& graph, const CookNodeVisitor& visitor, std::string& outErrorMessage) const = 0;
};
