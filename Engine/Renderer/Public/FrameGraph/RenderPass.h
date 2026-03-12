#pragma once

#include <string>
#include <string_view>

class PassBuilder;
class RenderContext;
struct SceneView;

class RenderPass
{
  public:
	explicit RenderPass(std::string_view name) noexcept : m_name(name) {}

	virtual ~RenderPass() noexcept = default;

	RenderPass(const RenderPass&) = delete;
	RenderPass& operator=(const RenderPass&) = delete;
	RenderPass(RenderPass&&) = delete;
	RenderPass& operator=(RenderPass&&) = delete;

	virtual void Setup(PassBuilder& builder, const SceneView& sceneView) = 0;

	virtual void Execute(RenderContext& context) = 0;

	const std::string& GetName() const noexcept { return m_name; }

  protected:
	std::string m_name;
};
