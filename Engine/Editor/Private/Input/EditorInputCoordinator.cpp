#include "PCH.h"
#include "Input/EditorInputCoordinator.h"

#include "UI.h"
#include "Input/Dispatch/InputLayer.h"
#include "Input/InputSystem.h"

EditorInputCoordinator::EditorInputCoordinator(InputSystem& inputSystem) noexcept : m_inputSystem(&inputSystem)
{
	m_inputSystem->SetAutomaticImGuiCaptureEnabled(false);
}

EditorInputCoordinator::~EditorInputCoordinator() noexcept
{
	if (m_inputSystem != nullptr)
	{
		m_inputSystem->SetLayerEnabled(InputLayer::Gameplay, true);
		m_inputSystem->SetAutomaticImGuiCaptureEnabled(true);
	}
}

void EditorInputCoordinator::UpdateGameplayInput(const UI& ui) noexcept
{
	if (m_inputSystem == nullptr)
	{
		return;
	}

	const bool enableGameplayInput = ui.WantsGameplayInput() || m_inputSystem->IsMouseCaptured();
	m_inputSystem->SetLayerEnabled(InputLayer::Gameplay, enableGameplayInput);
}
