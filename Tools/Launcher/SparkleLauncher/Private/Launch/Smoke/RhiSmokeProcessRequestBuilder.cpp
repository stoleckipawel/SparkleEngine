#include "Smoke/RhiSmokeProcessRequestBuilder.h"

#include <algorithm>
#include <utility>

namespace SparkleLauncher
{
	ProcessRequest BuildRhiSmokeBaseProcessRequest(const LaunchOperationPlan& plan)
	{
		ProcessRequest request;
		request.ExecutablePath = plan.ExecutablePath;
		request.WorkingDirectory = plan.WorkingDirectory;
		request.Environment = plan.Environment;
		request.Arguments.push_back("--graphics-api");
		request.Arguments.push_back("");
		AddRhiSmokeCVar(request, "r.VSync=false");
		return request;
	}

	void AddRhiSmokeCVar(ProcessRequest& request, std::string cvar)
	{
		request.Arguments.push_back("--cvar");
		request.Arguments.push_back(std::move(cvar));
	}

	void AddOrReplaceRhiSmokeEnvironment(ProcessRequest& request, std::string name, std::string value)
	{
		const auto found = std::find_if(
		    request.Environment.begin(),
		    request.Environment.end(),
		    [&name](const EnvironmentOverride& overrideValue)
		    {
			    return overrideValue.Name == name;
		    });
		if (found != request.Environment.end())
		{
			found->Value = std::move(value);
			return;
		}

		request.Environment.push_back(EnvironmentOverride{std::move(name), std::move(value)});
	}
}
