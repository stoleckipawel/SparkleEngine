#pragma once

namespace Assets
{
	struct LoadedSceneManifest;

	class SceneManifestValidator final
	{
	  public:
		static void ValidateHeader(const LoadedSceneManifest& manifest);
		static void ValidateRecords(const LoadedSceneManifest& manifest);
	};
}
