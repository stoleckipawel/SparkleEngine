#pragma once

// One optional action group in the viewport toolbar. UI installation transfers
// ownership; measurement, drawing, replacement and destruction run on the Editor
// thread with live ImGui. Destruction precedes backend/context teardown.
class ViewportToolbarActions
{
public:
	virtual ~ViewportToolbarActions() noexcept = default;
	ViewportToolbarActions(const ViewportToolbarActions&) = delete;
	ViewportToolbarActions& operator=(const ViewportToolbarActions&) = delete;
	ViewportToolbarActions(ViewportToolbarActions&&) = delete;
	ViewportToolbarActions& operator=(ViewportToolbarActions&&) = delete;

	virtual float MeasureWidth() const noexcept = 0;
	virtual void Draw(bool disableInteraction) noexcept = 0;

protected:
	ViewportToolbarActions() noexcept = default;
};
