#pragma once

#include <chrono>

struct RenderFrameTime final
{
	std::chrono::duration<double> UnscaledTime{};
	std::chrono::duration<double> ScaledTime{};
	std::chrono::duration<double> UnscaledDelta{};
	std::chrono::duration<double> ScaledDelta{};
};
