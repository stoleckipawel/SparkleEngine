#include "PCH.h"
#include "Diagnostics/ExternalCapture/ExternalCaptureControl.h"

void ArmExternalCapture(RhiExternalCapture& capture, const RenderExternalCaptureCommand& command, const ViewportRenderProducts& products) noexcept
{
	const auto snapshot = capture.Observe();
	if (snapshot.RequestId != command.RequestId)
	{
		return;
	}
	if (snapshot.ContextToken == 0 || products.GetGeneration() != snapshot.ContextToken || !products.HasOutput(RenderOutputFlags::FinalColorLdr))
	{
		capture.Reject(command.RequestId, "The requested viewport generation is stale. Wait for its current output and retry.");
		return;
	}
	capture.Arm(command.RequestId);
}

void ValidateExternalCaptureContext(RhiExternalCapture& capture, std::uint64_t currentGeneration, bool sceneReset) noexcept
{
	const auto snapshot = capture.Observe();
	if ((snapshot.State == ExternalCaptureState::Queued || snapshot.State == ExternalCaptureState::Capturing) && (snapshot.ContextToken != currentGeneration || sceneReset))
	{
		capture.InvalidateTarget();
	}
}
