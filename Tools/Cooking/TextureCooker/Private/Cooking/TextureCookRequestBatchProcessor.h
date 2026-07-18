#pragma once

#include "TextureCookRequestList.h"

#include <filesystem>
#include <string>
#include <vector>

class TextureCookRequestBatchProcessor final
{
  public:
	int CookRequestFile(const std::filesystem::path& requestFilePath) const;

  private:
	static bool TryLoadRequests(
	    const std::filesystem::path& requestFilePath,
	    std::vector<TextureCookRequest>& outRequests,
	    std::string& outErrorMessage);
};
