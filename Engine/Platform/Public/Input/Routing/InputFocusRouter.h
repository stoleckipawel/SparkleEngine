#pragma once

#include "Platform/Public/PlatformAPI.h"

#include "Input/Dispatch/InputLayer.h"
#include "Input/IInputBackend.h"
#include "Input/Mouse/MousePosition.h"

#include <vector>

class SPARKLE_PLATFORM_API InputFocusRouter final
{
  public:
	InputFocusRouter() noexcept = default;
	~InputFocusRouter() = default;

	InputFocusRouter(const InputFocusRouter&) = delete;
	InputFocusRouter& operator=(const InputFocusRouter&) = delete;
	InputFocusRouter(InputFocusRouter&&) = delete;
	InputFocusRouter& operator=(InputFocusRouter&&) = delete;

	void BeginFrame(bool interactionDisabled, bool textInputActive);
	void RegisterTargetRegion(float left, float top, float right, float bottom, InputLayer targetLayer);

	InputLayer ResolveTargetLayer(const InputBackendResult& result, InputLayer fallbackLayer) noexcept;
	InputLayer GetActiveTargetLayer() const noexcept;

  private:
	struct TargetRegion final
	{
		float Left = 0.0f;
		float Top = 0.0f;
		float Right = 0.0f;
		float Bottom = 0.0f;
		InputLayer TargetLayer = InputLayer::Editor;

		bool IsValid() const noexcept;
		bool Contains(MousePosition position) const noexcept;
	};

	InputLayer ResolvePointTarget(MousePosition position) const noexcept;
	InputLayer ResolveMouseMoveTarget(MousePosition position) const noexcept;

	std::vector<TargetRegion> m_targetRegions;
	InputLayer m_activeTargetLayer = InputLayer::Editor;
	bool m_hasRoutingFrame = false;
	bool m_interactionDisabled = false;
	bool m_textInputActive = false;
};