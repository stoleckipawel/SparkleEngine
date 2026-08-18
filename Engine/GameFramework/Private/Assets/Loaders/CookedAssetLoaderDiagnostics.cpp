#include "PCH.h"

#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"

#include <format>

namespace Assets
{
	CookedAssetLoaderDiagnostics::CookedAssetLoaderDiagnostics(
	    const std::filesystem::path& path,
	    std::string_view schemaName) :
	    m_path(path),
	    m_assetId(path.stem().generic_string()),
	    m_schemaName(schemaName)
	{
	}

	Diagnostics::Error CookedAssetLoaderDiagnostics::MakeError(
	    std::string_view recordKind,
	    std::string_view expectedFeature,
	    std::string_view reason) const
	{
		return Diagnostics::Error(std::format(
		    "Cooked asset load failed: asset='{}', path='{}', schema='{}', record='{}', expected='{}', reason='{}'",
		    m_assetId,
		    m_path.generic_string(),
		    m_schemaName,
		    recordKind,
		    expectedFeature,
		    reason));
	}
}
