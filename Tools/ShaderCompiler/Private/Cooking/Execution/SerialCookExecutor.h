#pragma once

#include "Cooking/Execution/ICookExecutor.h"

class SerialCookExecutor final : public ICookExecutor
{
  public:
	bool Execute(const DependencyGraph& graph, const CookNodeVisitor& visitor, std::string& outErrorMessage) const override;
};
