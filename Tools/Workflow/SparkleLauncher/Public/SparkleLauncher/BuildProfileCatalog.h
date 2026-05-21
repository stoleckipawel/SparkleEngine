#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class BuildProfileState
	{
		Debug,
		Development,
		Shipping
	};

	enum class BuildProfileTarget
	{
		Editor,
		Game
	};

	struct BuildProfile
	{
		std::string Name;
		BuildProfileState State = BuildProfileState::Development;
		BuildProfileTarget Target = BuildProfileTarget::Editor;
		std::string TargetSuffix;
	};

	const std::vector<BuildProfile>& GetBuildProfileCatalog();
	std::optional<BuildProfile> FindBuildProfile(std::string_view profileName);
	const BuildProfile& GetDefaultBuildProfile();
	bool IsEditorProfile(std::string_view profileName);
	std::string BuildProjectTargetName(std::string_view projectName, const BuildProfile& profile);
	std::string ToString(BuildProfileState state);
	std::string ToString(BuildProfileTarget target);
}