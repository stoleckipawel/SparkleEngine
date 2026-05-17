#pragma once

struct ImDrawData;

class VulkanImGuiBackend final
{
  public:
	bool Initialize();
	void BeginFrame() noexcept;
	void RenderDrawData(ImDrawData* drawData) noexcept;
	void Shutdown() noexcept;
};