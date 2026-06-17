#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Renderer.h"
#include "RuntimeApplication.h"
#include "Validation/RhiSmokeSession.h"

class RhiSmokeValidationRunner final
{
  public:
	static bool IsRequested() noexcept;
	static int RunProject() noexcept;
	static int RunProject(RuntimeApplicationOptions options) noexcept;

  private:
	static bool TickRuntime(RuntimeApplication& app, const RhiSmokeSessionConfig& config, RhiSmokeSessionState& state) noexcept;
	static int RunProjectValidation(const RhiSmokeSessionConfig& config, RuntimeApplicationOptions options) noexcept;
};

bool RhiSmokeValidationRunner::TickRuntime(
    RuntimeApplication& app,
    const RhiSmokeSessionConfig& config,
    RhiSmokeSessionState& state) noexcept
{
	switch (app.BeginFrame())
	{
		case RuntimeApplicationFrameResult::Exit:
			return false;
		case RuntimeApplicationFrameResult::SkipRender:
			return true;
		case RuntimeApplicationFrameResult::Ready:
		default:
			break;
	}

	app.UpdateRuntime();
	app.GetRenderer().OnRender();
	RhiSmokeSession::LogRendererEvidence(config, app, state, "RHI runtime smoke evidence", "RHI runtime smoke validation");
	app.EndFrame();
	RhiSmokeFrameControl::Advance(config.FrameControl, app, state.FrameControl, "runtime");
	return true;
}

int RhiSmokeValidationRunner::RunProjectValidation(const RhiSmokeSessionConfig& config, RuntimeApplicationOptions options) noexcept
{
	RuntimeApplication app(std::move(options));
	RhiSmokeSessionState state{};
	RhiSmokeSession::ApplyLoggingConfig(config);
	app.Initialize();
	RhiSmokeSession::LogDiagnosticsCapabilities(config, app, state);
	RhiSmokeSession::InitializeFrameControl(config, app, state);

	while (TickRuntime(app, config, state))
	{
	}

	app.Shutdown();
	return state.FrameControl.Failed ? 1 : 0;
}

bool RhiSmokeValidationRunner::IsRequested() noexcept
{
	return RhiSmokeSession::LoadConfig().Enabled;
}

int RhiSmokeValidationRunner::RunProject() noexcept
{
	return RunProjectValidation(RhiSmokeSession::LoadConfig(), RuntimeApplicationOptions{});
}

int RhiSmokeValidationRunner::RunProject(RuntimeApplicationOptions options) noexcept
{
	return RunProjectValidation(RhiSmokeSession::LoadConfig(), std::move(options));
}

bool RhiSmokeValidation::IsRequested() noexcept
{
	return RhiSmokeValidationRunner::IsRequested();
}

int RhiSmokeValidation::RunProject() noexcept
{
	return RhiSmokeValidationRunner::RunProject();
}

int RhiSmokeValidation::RunProject(RuntimeApplicationOptions options) noexcept
{
	return RhiSmokeValidationRunner::RunProject(std::move(options));
}
