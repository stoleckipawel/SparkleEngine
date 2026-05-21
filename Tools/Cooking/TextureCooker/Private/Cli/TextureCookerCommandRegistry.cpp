#include "PCH.h"

#include "Cli/TextureCookerCommandRegistry.h"

#include "Cli/CookTextureCookRequestFileCommand.h"
#include "Cli/InspectTextureCookRequestFileCommand.h"

#include <memory>

	std::unique_ptr<TextureCookerCommand> TextureCookerCommandRegistry::Create(std::string_view commandName)
	{
		if (InspectTextureCookRequestFileCommand::MatchesName(commandName))
		{
			return std::make_unique<InspectTextureCookRequestFileCommand>();
		}

		if (CookTextureCookRequestFileCommand::MatchesName(commandName))
		{
			return std::make_unique<CookTextureCookRequestFileCommand>();
		}

		return nullptr;
	}

	void TextureCookerCommandRegistry::PrintUsage(std::ostream& output)
	{
		output << "Usage:\n"
		       << "  TextureCooker inspect-request-file <request-file-path>\n"
		       << "  TextureCooker cook-request-file <request-file-path> [--summary <summary-json-path>]\n";
	}