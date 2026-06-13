#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace Assets
{
	struct CookedAssetLoaderContext
	{
		std::filesystem::path path;
		std::string assetId;
		std::string_view schemaName = {};
		std::uint32_t schemaVersion = 0;
	};

	class CookedAssetLoaderDiagnostics final
	{
	  public:
		CookedAssetLoaderDiagnostics() = delete;

		static CookedAssetLoaderContext BuildContext(
		    const std::filesystem::path& path,
		    std::string_view schemaName,
		    std::uint32_t schemaVersion);

		static void SetFailure(
		    const CookedAssetLoaderContext& context,
		    std::string_view recordKind,
		    std::string_view expectedFeature,
		    std::string_view reason,
		    std::string& outErrorMessage);
	};
}
