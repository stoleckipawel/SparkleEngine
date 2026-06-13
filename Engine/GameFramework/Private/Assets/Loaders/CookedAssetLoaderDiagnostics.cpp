#include "PCH.h"

#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"

#include <format>

namespace Assets
{
	CookedAssetLoaderContext CookedAssetLoaderDiagnostics::BuildContext(
	    const std::filesystem::path& path,
	    std::string_view schemaName,
	    std::uint32_t schemaVersion)
	{
		return {.path = path, .assetId = path.stem().generic_string(), .schemaName = schemaName, .schemaVersion = schemaVersion};
	}

	void CookedAssetLoaderDiagnostics::SetFailure(
	    const CookedAssetLoaderContext& context,
	    std::string_view recordKind,
	    std::string_view expectedFeature,
	    std::string_view reason,
	    std::string& outErrorMessage)
	{
		outErrorMessage = std::format(
		    "Cooked asset load failed: asset='{}', path='{}', schema='{}', version={}, record='{}', expected='{}', reason='{}'",
		    context.assetId,
		    context.path.generic_string(),
		    context.schemaName,
		    context.schemaVersion,
		    recordKind,
		    expectedFeature,
		    reason);
	}
}
