#pragma once

#include "SceneCooker.h"
#include "SourceImportResult.h"
#include "TextureCookRequestList.h"

#include <filesystem>
#include <string>
#include <vector>

class CookedSceneCooker final
{
public:
	CookedSceneBuild Cook(const std::filesystem::path& sourceScenePath, const SourceImportResult& importResult) const;
	bool CollectTextureCookRequests(
	    const SourceImportResult& importResult,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage) const;
};
