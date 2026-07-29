#pragma once

#include "TextureCookRequestList.h"

#include <filesystem>
#include <string>
#include <vector>

struct TextureCookBatchItemResult;

class TextureCookRequestBatchProcessor final
{
  public:
	int CookRequestFile(const std::filesystem::path& requestFilePath) const;

  private:
	static bool ReportFailures(
	    const std::vector<TextureCookRequest>& requests,
	    const std::vector<TextureCookBatchItemResult>& results,
	    bool batchSucceeded);
	static bool PublishGeneration(
	    const std::vector<TextureCookRequest>& requests,
	    const std::vector<TextureCookBatchItemResult>& results,
	    std::string& outErrorMessage);
	static void CleanupStagedOutputs(
	    const std::vector<TextureCookBatchItemResult>& results);
};
