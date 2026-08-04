#pragma once

#include "LauncherCapabilityRegistry.h"

#include <QtCore/QString>

#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	struct LauncherLevelUiModel;
	struct LauncherCapabilityId final
	{
		inline static constexpr std::string_view HostTools = "workspace.host-tools";
		inline static constexpr std::string_view SourceDependencies = "workspace.source-dependencies";
		inline static constexpr std::string_view BuildFiles = "workspace.build-files";
		inline static constexpr std::string_view SelectedLevels = "content.selected-levels";
		inline static constexpr std::string_view CookingTools = "content.cooking-tools";
		inline static constexpr std::string_view CookedContent = "content.cooked";
	};

	struct LauncherCapabilityContext
	{
		LauncherOperationRequest Request;
		const LauncherLevelUiModel& LevelModel;

		bool IsRuntimeProduct() const;
		QString ProductBuildOperationId() const;
		std::string ProductCapabilityId() const;
		std::string ProjectCapabilityId() const;
	};

	using LauncherCapabilityProvider = std::string (*)(LauncherCapabilityRegistry&, const LauncherCapabilityContext&);

	std::string BuildCapabilityReadinessSummary(const std::vector<std::string>& messages);
	std::string RegisterWorkspaceCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context);
	std::string RegisterLevelCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context);
	std::string RegisterCookCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context);
	std::string RegisterLaunchCapabilities(LauncherCapabilityRegistry& registry, const LauncherCapabilityContext& context);
}
