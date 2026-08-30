#include "PCH.h"

#include "FrameGraph/Compiler/FrameGraphRecordingPlan.h"

bool SubmissionOrderKey::operator==(const SubmissionOrderKey&) const noexcept = default;

void RecordingPlan::Clear() noexcept
{
	Passes.clear();
	Groups.clear();
	Chunks.clear();
}
