#pragma once

class RhiSmokeValidation final
{
  public:
	static bool IsRequested() noexcept;
	static int RunProject() noexcept;
	static int RunEditor() noexcept;
};
