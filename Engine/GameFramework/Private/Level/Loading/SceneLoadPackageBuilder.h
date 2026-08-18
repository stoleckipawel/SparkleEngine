#pragma once

#include <cstddef>

namespace Assets
{
	struct SceneAssetLoadWork;
	struct SceneLoadWorkState;

	class SceneLoadPackageBuilder final
	{
	public:
		static void BuildAssetBlueprints(SceneAssetLoadWork& work);
		static void Finalize(SceneLoadWorkState& state);
	};
}
