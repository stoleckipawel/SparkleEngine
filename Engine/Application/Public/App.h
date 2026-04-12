#pragma once

#include "Application/Public/ApplicationAPI.h"

#include <memory>

class ProjectApp;

class SPARKLE_APPLICATION_API App
{
  public:
	App();
	~App();

	App(const App&) = delete;
	App& operator=(const App&) = delete;
	App(App&&) = delete;
	App& operator=(App&&) = delete;

	void Run();

  private:
	std::unique_ptr<ProjectApp> m_projectApp;
};
