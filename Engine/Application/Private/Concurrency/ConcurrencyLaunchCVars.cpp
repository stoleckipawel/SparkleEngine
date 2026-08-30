#include "PCH.h"

#include "Concurrency/ConcurrencyLaunchCVars.h"

#include "Core/Public/Console/CVar.h"

namespace ConcurrencyLaunchCVars
{
	ConsoleVariable<bool> g_threadedRenderer("r.ThreadedRenderer", true, "Run renderer/RHI ownership on Sparkle.RenderThread.");
	ConsoleVariable<std::uint32_t> g_renderPipelineDepth(
	    "r.RenderPipelineDepth",
	    1,
	    "Bounded renderer CPU lead in frames. Supported values are 0 through 2.");

	void Register() noexcept
	{
	}
	bool UseThreadedRenderer() noexcept
	{
		return g_threadedRenderer.Get();
	}
	std::uint32_t ResolveRenderPipelineDepth() noexcept
	{
		const std::uint32_t renderPipelineDepth = g_renderPipelineDepth.Get();
		if (renderPipelineDepth > 2u)
		{
			Diagnostics::Fatal(
			    Logging::GetOrCreateLogger("Application.Concurrency"),
			    __FILE__,
			    __LINE__,
			    "r.RenderPipelineDepth is outside the supported range [0, 2].");
		}
		return renderPipelineDepth;
	}
}
