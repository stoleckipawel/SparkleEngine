#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include "Core/Public/Events/Event.h"

#include <string>

struct SPARKLE_ENGINE_API LevelChangeStartedEventArgs
{
	std::string previousLevelName;
	std::string requestedLevelName;
};

struct SPARKLE_ENGINE_API LevelWillUnloadEventArgs
{
	std::string previousLevelName;
	std::string requestedLevelName;
};

struct SPARKLE_ENGINE_API LevelUnloadedEventArgs
{
	std::string previousLevelName;
};

struct SPARKLE_ENGINE_API LevelWillLoadEventArgs
{
	std::string targetLevelName;
};

struct SPARKLE_ENGINE_API LevelChangedEventArgs
{
	std::string previousLevelName;
	std::string activeLevelName;
};

struct SPARKLE_ENGINE_API LevelLoadFailedEventArgs
{
	std::string failedLevelName;
	std::string fallbackLevelName;
};

class SPARKLE_ENGINE_API LevelChangeEvents final
{
  public:
	Event<void(const LevelChangeStartedEventArgs&), 16> OnLevelChangeStarted;
	Event<void(const LevelWillUnloadEventArgs&), 16> OnLevelWillUnload;
	Event<void(const LevelUnloadedEventArgs&), 16> OnLevelUnloaded;
	Event<void(const LevelWillLoadEventArgs&), 16> OnLevelWillLoad;
	Event<void(const LevelChangedEventArgs&), 16> OnLevelChanged;
	Event<void(const LevelLoadFailedEventArgs&), 16> OnLevelLoadFailed;
};