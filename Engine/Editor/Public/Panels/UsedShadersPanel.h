#pragma once

#include "Shaders/RegisteredShaderListModel.h"

#include <array>
#include <functional>
#include <string>

class UsedShadersPanel final
{
  public:
	using RecookHandler = std::function<void(std::string)>;
	using InspectHandler = std::function<void()>;

	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }
	void SetGenerationProvider(RegisteredShaderListModel::GenerationProvider provider);
	void SetRecookHandler(RecookHandler handler);
	void SetInspectHandler(InspectHandler handler);
	void SetLastStatus(std::string status);
	void BuildUI(bool disableInteraction);

  private:
	void EnsureRows();
	void DrawToolbar(bool disableInteraction);
	void DrawTable(bool disableInteraction);
	const RegisteredShaderRow* GetSelectedRow() const noexcept;
	bool MatchesFilter(const RegisteredShaderRow& row) const noexcept;

	RegisteredShaderListModel m_model;
	RecookHandler m_recookHandler;
	InspectHandler m_inspectHandler;
	std::array<char, 160> m_filterBuffer{};
	std::string m_selectedShaderId;
	bool m_hasRows = false;
	bool m_isOpen = false;
};
