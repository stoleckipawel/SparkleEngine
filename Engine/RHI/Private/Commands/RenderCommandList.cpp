#include "PCH.h"

#include "Commands/RenderCommandList.h"

RenderCommandList::~RenderCommandList() noexcept = default;

void RenderCommandList::TrackResource(RhiResourceHandle resource)
{
	if (!resource)
	{
		return;
	}
	for (const RhiResourceHandle tracked : m_trackedResources)
	{
		if (tracked.Value == resource.Value)
		{
			return;
		}
	}

	m_trackedResources.push_back(resource);
	OnResourceTrackingStarted(resource);
}

void RenderCommandList::ResolveTrackedResources(RhiSubmissionToken submissionToken) noexcept
{
	for (const RhiResourceHandle resource : m_trackedResources)
	{
		OnResourceTrackingFinished(resource, submissionToken);
	}
	m_trackedResources.clear();
}

void RenderCommandList::ResetTrackedResources() noexcept
{
	ResolveTrackedResources({});
}
