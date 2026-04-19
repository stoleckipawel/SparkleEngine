#pragma once

class D3D12SmokeValidation final
{
  public:
	static bool IsRequested() noexcept;
	static int RunProject() noexcept;
	static int RunEditor() noexcept;
};