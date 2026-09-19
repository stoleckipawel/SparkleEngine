#pragma once

#include <cstdint>
#include <memory>

class EditorOperationRuntime;
class Renderer;
class UI;

class EditorViewportOutputCoordinator final
{
public:
	explicit EditorViewportOutputCoordinator(EditorOperationRuntime& operations) noexcept;
	~EditorViewportOutputCoordinator() noexcept;

	EditorViewportOutputCoordinator(const EditorViewportOutputCoordinator&) = delete;
	EditorViewportOutputCoordinator& operator=(const EditorViewportOutputCoordinator&) = delete;

	void Update(Renderer& renderer);
	void HandleAction(UI& ui, Renderer& renderer, std::uint64_t frameId);

private:
	struct State;
	std::unique_ptr<State> m_state;
};
