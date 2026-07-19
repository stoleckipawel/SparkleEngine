#pragma once

#include <cstddef>
#include <string>

namespace Assets
{
	struct SceneAssetLoadWork;
	struct SceneLoadSharedState;

	class SceneLoadPackageBuilder final
	{
	  public:
		static bool BuildAssetBlueprints(
		    SceneAssetLoadWork& work,
		    std::size_t& decodedBytes,
		    std::string& errorMessage);
		static bool Finalize(SceneLoadSharedState& state, std::string& errorMessage);
	};
}
