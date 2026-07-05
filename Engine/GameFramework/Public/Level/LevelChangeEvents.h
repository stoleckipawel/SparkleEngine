#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include "Core/Public/Events/Event.h"

#include <string>
#include <string_view>

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

struct SPARKLE_ENGINE_API LevelChangedEventArgs
{
	std::string previousLevelName;
	std::string activeLevelName;
};

class SPARKLE_ENGINE_API LevelChangeEvents final
{
  public:
	Event<void(const LevelChangeStartedEventArgs&), 16> OnLevelChangeStarted;
	Event<void(const LevelWillUnloadEventArgs&), 16> OnLevelWillUnload;
	Event<void(std::string_view), 16> OnLevelUnloaded;
	Event<void(std::string_view), 16> OnLevelWillLoad;
	Event<void(const LevelChangedEventArgs&), 16> OnLevelChanged;
	Event<void(std::string_view), 16> OnLevelLoadFailed;
};
