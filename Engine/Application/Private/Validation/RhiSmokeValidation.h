#pragma once

#include "EditorApplication.h"
#include "RuntimeApplication.h"

class RhiSmokeValidation final
{
  public:
	static bool IsRequested() noexcept;
	static int RunProject() noexcept;
	static int RunProject(RuntimeApplicationOptions options) noexcept;
	static int RunEditor() noexcept;
	static int RunEditor(EditorApplicationOptions options) noexcept;
};
