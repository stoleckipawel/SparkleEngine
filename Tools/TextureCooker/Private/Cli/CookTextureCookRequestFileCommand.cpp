#include "PCH.h"

#include "Cli/CookTextureCookRequestFileCommand.h"

#include "Constants/TextureCookerConstants.h"
#include "Cooking/TextureCookRequestBatchProcessor.h"

	bool CookTextureCookRequestFileCommand::MatchesName(std::string_view commandName) noexcept
	{
		return commandName == TextureCookerConstants::CookRequestFileCommand;
	}

	int CookTextureCookRequestFileCommand::Execute(
		const std::filesystem::path& requestFilePath,
		const TextureCookerCommandOptions& options) const
	{
		TextureCookRequestBatchProcessor batchProcessor;
		return batchProcessor.CookRequestFile(requestFilePath, options.summaryPath);
	}