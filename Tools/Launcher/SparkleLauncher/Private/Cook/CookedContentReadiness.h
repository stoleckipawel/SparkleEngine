#pragma once

#include <cstdint>
#include <filesystem>
#include <string_view>

namespace SparkleLauncher
{
	enum class CookedShaderPublicationState : std::uint8_t
	{
		Missing,
		Invalid,
		Ready
	};

	struct CookedContentReadiness
	{
		bool MeshesReady = false;
		bool TexturesReady = false;
		CookedShaderPublicationState Shaders = CookedShaderPublicationState::Missing;
	};

	CookedContentReadiness InspectCookedContentReadiness(const std::filesystem::path& repositoryRoot, std::string_view projectId);
}
