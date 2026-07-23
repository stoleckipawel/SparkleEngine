#include "PCH.h"

#include "Concurrency/ConcurrencyLaunchCVars.h"

#include "Core/Public/Console/CVar.h"

namespace ConcurrencyLaunchCVars
{
	ConsoleVariable<bool> g_threadedRenderer(
	    "r.ThreadedRenderer", false, "Run renderer/RHI ownership on Sparkle.RenderThread.");
	ConsoleVariable<bool> g_parallelCommandRecording(
	    "r.ParallelCommandRecording", false, "Development switch reserved for future parallel RHI command recording.");
	ConsoleVariable<std::uint32_t> g_renderPipelineDepth(
	    "r.RenderPipelineDepth", 0, "Bounded renderer CPU lead: 0 is zero-ahead, 1 is one-ahead.");

	void Register() noexcept {}
	bool UseThreadedRenderer() noexcept { return g_threadedRenderer.Get(); }
	std::uint32_t ResolveRenderPipelineDepth() noexcept { return (std::min)(g_renderPipelineDepth.Get(), 1u); }
}
