#include "PCH.h"

#include "Cooking/Execution/SerialCookExecutor.h"

bool SerialCookExecutor::Execute(
	const DependencyGraph& graph,
	const CookNodeVisitor& visitor,
	std::string& outErrorMessage) const
{
	for (const CookNode& node : graph.GetTopologicalOrder())
	{
		if (!visitor(node, outErrorMessage))
		{
			return false;
		}
	}

	outErrorMessage.clear();
	return true;
}
