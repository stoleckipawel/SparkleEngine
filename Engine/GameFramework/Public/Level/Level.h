#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Level/LevelDesc.h"

#include <string_view>

class SPARKLE_ENGINE_API Level
{
  public:
	virtual ~Level() = default;

	Level(const Level&) = delete;
	Level& operator=(const Level&) = delete;
	Level(Level&&) = delete;
	Level& operator=(Level&&) = delete;

	virtual std::string_view GetName() const = 0;

	virtual std::string_view GetDescription() const = 0;

	virtual LevelDesc BuildDescription() const = 0;

  protected:
	Level() = default;
};
