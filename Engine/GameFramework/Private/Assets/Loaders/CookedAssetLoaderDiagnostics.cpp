#include "PCH.h"

#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"

#include <format>

namespace Assets
{
	CookedAssetLoaderDiagnostics::CookedAssetLoaderDiagnostics(
	    const std::filesystem::path& path,
	    std::string_view schemaName,
	    std::uint32_t schemaVersion) :
	    m_path(path),
	    m_assetId(path.stem().generic_string()),
	    m_schemaName(schemaName),
	    m_schemaVersion(schemaVersion)
	{
	}

	Diagnostics::Error CookedAssetLoaderDiagnostics::MakeError(
	    std::string_view recordKind,
	    std::string_view expectedFeature,
	    std::string_view reason) const
	{
		return Diagnostics::Error(std::format(
		    "Cooked asset load failed: asset='{}', path='{}', schema='{}', version={}, record='{}', expected='{}', reason='{}'",
		    m_assetId,
		    m_path.generic_string(),
		    m_schemaName,
		    m_schemaVersion,
		    recordKind,
		    expectedFeature,
		    reason));
	}
}
