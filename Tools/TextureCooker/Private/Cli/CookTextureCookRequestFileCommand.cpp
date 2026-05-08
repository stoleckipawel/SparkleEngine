#include "PCH.h"

#include "Cli/CookTextureCookRequestFileCommand.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureCookRequestBatchProcessor.h"

namespace AssetAuthoring
{
	bool CookTextureCookRequestFileCommand::MatchesName(std::string_view commandName) noexcept
	{
		return commandName == TextureCookerConstants::CookRequestFileCommand ||
		       commandName == TextureCookerConstants::CookRequestFileAlias;
	}

	int CookTextureCookRequestFileCommand::Execute(const std::filesystem::path& requestFilePath) const
	{
		TextureCookRequestBatchProcessor batchProcessor;
		return batchProcessor.CookRequestFile(requestFilePath);
	}
}