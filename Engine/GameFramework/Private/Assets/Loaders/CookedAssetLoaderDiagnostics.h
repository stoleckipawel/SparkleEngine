#pragma once

#include "Core/Public/Diagnostics/Error.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace Assets
{
	class CookedAssetLoaderDiagnostics final
	{
	  public:
		CookedAssetLoaderDiagnostics(
		    const std::filesystem::path& path,
		    std::string_view schemaName);

		Diagnostics::Error MakeError(std::string_view recordKind, std::string_view expectedFeature, std::string_view reason) const;

	  private:
		std::filesystem::path m_path;
		std::string m_assetId;
		std::string m_schemaName;
	};
}
