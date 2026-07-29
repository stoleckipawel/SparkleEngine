#pragma once

#include <cstddef>
namespace Assets
{
	struct SceneAssetLoadWork;
	struct SceneLoadWorkState;

	class SceneLoadPackageBuilder final
	{
	  public:
		static std::size_t BuildAssetBlueprints(SceneAssetLoadWork& work);
		static void Finalize(SceneLoadWorkState& state);
	};
}
