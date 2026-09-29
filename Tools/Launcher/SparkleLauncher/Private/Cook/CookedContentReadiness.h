#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace SparkleLauncher
{
	enum class CookedOutputState : std::uint8_t
	{
		Missing,
		Stale,
		Ready
	};

	struct CookedContentReadiness
	{
		bool MeshesReady = false;
		bool TexturesReady = false;
		CookedOutputState Shaders = CookedOutputState::Missing;
	};

	CookedContentReadiness InspectCookedContentReadiness(
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectId);
}
