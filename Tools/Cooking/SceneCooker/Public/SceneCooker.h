#pragma once

#include <filesystem>
#include <span>
#include <vector>

namespace Files
{
	struct FilePublication;
}

struct CookedSceneBuild;
struct CookedSceneIdentity;
struct SourceImportOutput;
class CookedSceneGenerationWriter;

class SceneCooker final
{
public:
	static CookedSceneIdentity ResolveSceneIdentity(const std::filesystem::path& sourceScenePath);
	static void BuildManifest(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild);

private:
	friend class CookedSceneGenerationWriter;

	static void StageManifestsAndRegistry(
	    std::span<const CookedSceneBuild* const> builds,
	    std::vector<Files::FilePublication>& outPublication);
};
