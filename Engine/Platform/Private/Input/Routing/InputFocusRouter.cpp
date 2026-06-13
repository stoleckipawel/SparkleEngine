#include "PCH.h"
#include "Input/Routing/InputFocusRouter.h"

void InputFocusRouter::BeginFrame(bool interactionDisabled, bool textInputActive)
{
	m_targetRegions.clear();
	m_hasRoutingFrame = true;
	m_interactionDisabled = interactionDisabled;
	m_textInputActive = textInputActive;
	if (m_interactionDisabled)
	{
		m_activeTargetLayer = InputLayer::UI;
	}
}

void InputFocusRouter::RegisterTargetRegion(float left, float top, float right, float bottom, InputLayer targetLayer)
{
	TargetRegion region{};
	region.Left = left;
	region.Top = top;
	region.Right = right;
	region.Bottom = bottom;
	region.TargetLayer = targetLayer;
	if (region.IsValid())
	{
		m_targetRegions.push_back(region);
	}
}

InputLayer InputFocusRouter::ResolveTargetLayer(const InputBackendResult& result, InputLayer fallbackLayer) noexcept
{
	if (!m_hasRoutingFrame)
	{
		return fallbackLayer;
	}

	if (m_interactionDisabled)
	{
		return InputLayer::UI;
	}

	switch (result.Type)
	{
		case InputEventType::MouseButton:
			if (result.MouseButton.bPressed)
			{
				m_activeTargetLayer = ResolvePointTarget(result.MouseButton.Position);
				if (m_activeTargetLayer == InputLayer::Gameplay)
				{
					m_textInputActive = false;
				}
				return m_activeTargetLayer;
			}
			break;
		case InputEventType::MouseMove:
			return ResolveMouseMoveTarget(result.MouseMove.Position);
		case InputEventType::MouseWheel:
			return ResolvePointTarget(result.MouseWheel.Position);
		case InputEventType::Keyboard:
			if (m_textInputActive && m_activeTargetLayer != InputLayer::Gameplay)
			{
				return InputLayer::UI;
			}
			return GetActiveTargetLayer();
		default:
			break;
	}

	return GetActiveTargetLayer() != InputLayer::System ? GetActiveTargetLayer() : fallbackLayer;
}

InputLayer InputFocusRouter::GetActiveTargetLayer() const noexcept
{
	if (!m_hasRoutingFrame)
	{
		return InputLayer::System;
	}

	if (m_interactionDisabled)
	{
		return InputLayer::UI;
	}

	return m_activeTargetLayer;
}

bool InputFocusRouter::TargetRegion::IsValid() const noexcept
{
	return TargetLayer != InputLayer::System && Right > Left && Bottom > Top;
}

bool InputFocusRouter::TargetRegion::Contains(MousePosition position) const noexcept
{
	const float x = static_cast<float>(position.X);
	const float y = static_cast<float>(position.Y);
	return x >= Left && x < Right && y >= Top && y < Bottom;
}

InputLayer InputFocusRouter::ResolvePointTarget(MousePosition position) const noexcept
{
	for (auto region = m_targetRegions.rbegin(); region != m_targetRegions.rend(); ++region)
	{
		if (region->Contains(position))
		{
			return region->TargetLayer;
		}
	}

	return InputLayer::UI;
}

InputLayer InputFocusRouter::ResolveMouseMoveTarget(MousePosition position) const noexcept
{
	if (m_activeTargetLayer != InputLayer::Gameplay)
	{
		return InputLayer::UI;
	}

	return ResolvePointTarget(position) == InputLayer::Gameplay ? InputLayer::Gameplay : InputLayer::UI;
}
