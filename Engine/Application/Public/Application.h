#pragma once

#include "ApplicationAPI.h"

class SPARKLE_APPLICATION_API Application
{
  public:
	virtual ~Application() = default;

	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application&&) = delete;

	static void ConfigureProcessFromCommandLine() noexcept;

	void Run();
	virtual void Initialize() = 0;
	virtual bool Tick() = 0;
	virtual void Shutdown() = 0;

  protected:
	Application() = default;
};
