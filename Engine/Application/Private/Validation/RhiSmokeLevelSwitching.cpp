#include "PCH.h"

#include "Validation/RhiSmokeLevelSwitching.h"

#include "Level/Level.h"
#include "Level/LevelManager.h"
#include "RuntimeApplication.h"

#include <algorithm>

namespace
{
	std::shared_ptr<spdlog::logger> GetSmokeLogger()
	{
		return Logging::GetOrCreateLogger("Application.SmokeValidation");
	}

	std::string GetActiveLevelName(const RuntimeApplication& app)
	{
		const LevelManager* levelManager = app.GetLevelManager();
		if (levelManager == nullptr)
		{
			return {};
		}

		const LevelAsset* activeLevel = levelManager->GetActiveLevel();
		return activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string();
	}
}

namespace RhiSmokeLevelSwitching
{
	void Initialize(
	    const RhiSmokeLevelSwitchingConfig& config,
	    RuntimeApplication& app,
	    RhiSmokeLevelSwitchingState& state,
	    bool& failed) noexcept
	{
		if (!config.Enabled || state.Initialized)
		{
			return;
		}

		state.Initialized = true;
		const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
		LevelManager* levelManager = app.GetLevelManager();
		if (levelManager == nullptr)
		{
			failed = true;
			SPDLOG_LOGGER_ERROR(logger, "RHI smoke validation: level switching requested but no LevelManager is available");
			return;
		}

		state.SwitchOrder = levelManager->GetRegisteredLevelNames();
		const std::string activeLevelName = GetActiveLevelName(app);
		state.SwitchOrder.erase(std::remove(state.SwitchOrder.begin(), state.SwitchOrder.end(), activeLevelName), state.SwitchOrder.end());


		if (state.SwitchOrder.empty())
		{
			state.Finished = true;
		}
	}

	void Advance(
	    const RhiSmokeLevelSwitchingConfig& config,
	    RuntimeApplication& app,
	    std::uint32_t completedRenderFrames,
	    RhiSmokeLevelSwitchingState& state,
	    bool& failed) noexcept
	{
		if (!config.Enabled || state.Finished)
		{
			return;
		}

		Initialize(config, app, state, failed);

		const std::shared_ptr<spdlog::logger> logger = GetSmokeLogger();
		const std::string activeLevelName = GetActiveLevelName(app);
		if (!state.PendingLevelName.empty() && activeLevelName == state.PendingLevelName)
		{
			++state.CompletedSwitches;
			state.PendingLevelName.clear();
			state.LastSwitchFrame = completedRenderFrames;
		}

		if (state.PendingLevelName.empty() && state.CompletedSwitches >= state.SwitchOrder.size())
		{
			state.Finished = true;
			return;
		}

		if (!state.PendingLevelName.empty())
		{
			return;
		}

		const std::uint32_t interval = std::max<std::uint32_t>(config.IntervalFrames, 1u);
		if (completedRenderFrames - state.LastSwitchFrame < interval)
		{
			return;
		}

		const std::string& nextLevelName = state.SwitchOrder[state.CompletedSwitches];
		LevelManager* levelManager = app.GetLevelManager();
		if (levelManager == nullptr)
		{
			failed = true;
			SPDLOG_LOGGER_ERROR(logger, "RHI smoke validation: lost LevelManager before requesting level switch to '{}'", nextLevelName);
			return;
		}

		state.PendingLevelName = nextLevelName;
		levelManager->RequestLevelChange(nextLevelName);
	}
}
