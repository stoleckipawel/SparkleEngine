#include "PCH.h"

#include "Cli/CookTextureCookRequestFileCommand.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureCookRequestBatchProcessor.h"

int CookTextureCookRequestFile(const std::filesystem::path& requestFilePath)
{
	TextureCookRequestBatchProcessor batchProcessor;
	return batchProcessor.CookRequestFile(requestFilePath);
}
