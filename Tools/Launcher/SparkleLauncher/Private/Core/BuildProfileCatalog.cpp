#include "SparkleLauncher/BuildProfileCatalog.h"

#include <algorithm>

namespace SparkleLauncher
{
	const std::vector<BuildProfile>& GetBuildProfileCatalog()
	{
		static const std::vector<BuildProfile> profiles = {
		    {"DevelopmentEditor", BuildProfileState::Development, BuildProfileTarget::Editor, "Editor"},
		    {"DevelopmentGame", BuildProfileState::Development, BuildProfileTarget::Game, "Runtime"},
		    {"DebugEditor", BuildProfileState::Debug, BuildProfileTarget::Editor, "Editor"},
		    {"DebugGame", BuildProfileState::Debug, BuildProfileTarget::Game, "Runtime"},
		    {"ShippingEditor", BuildProfileState::Shipping, BuildProfileTarget::Editor, "Editor"},
		    {"ShippingGame", BuildProfileState::Shipping, BuildProfileTarget::Game, "Runtime"},
		};
		return profiles;
	}

	std::optional<BuildProfile> FindBuildProfile(std::string_view profileName)
	{
		const auto& profiles = GetBuildProfileCatalog();
		const auto found = std::find_if(
		    profiles.begin(),
		    profiles.end(),
		    [profileName](const BuildProfile& profile) { return profile.Name == profileName; });
		if (found == profiles.end())
		{
			return std::nullopt;
		}

		return *found;
	}

	const BuildProfile& GetDefaultBuildProfile()
	{
		return GetBuildProfileCatalog().front();
	}

	bool IsEditorProfile(std::string_view profileName)
	{
		const std::optional<BuildProfile> profile = FindBuildProfile(profileName);
		return profile.has_value() && profile->Target == BuildProfileTarget::Editor;
	}

	std::string BuildProjectTargetName(std::string_view projectName, const BuildProfile& profile)
	{
		std::string targetName(projectName);
		targetName += profile.TargetSuffix;
		return targetName;
	}

	std::string ToString(BuildProfileState state)
	{
		switch (state)
		{
			case BuildProfileState::Debug:
				return "Debug";
			case BuildProfileState::Development:
				return "Development";
			case BuildProfileState::Shipping:
				return "Shipping";
		}

		return "Unknown";
	}

	std::string ToString(BuildProfileTarget target)
	{
		switch (target)
		{
			case BuildProfileTarget::Editor:
				return "Editor";
			case BuildProfileTarget::Game:
				return "Game";
		}

		return "Unknown";
	}
}
