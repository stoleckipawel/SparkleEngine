#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"
#include "World/ECS/Components/AnimationComponents.h"

#include <vector>
#include <utility>

namespace ECS
{
	class SceneAnimationResources final
	{
	  public:
		AnimationResourceHandle Add(SceneAnimationClipDesc&& clip);
		const SceneAnimationClipDesc* Resolve(AnimationResourceHandle handle) const noexcept;
		void SetDerivedOutput(SceneAnimationSnapshot&& output) noexcept { m_derivedOutput = std::move(output); }
		const SceneAnimationSnapshot& GetDerivedOutput() const noexcept { return m_derivedOutput; }
		void Clear() noexcept;

	  private:
		struct Entry final
		{
			SceneAnimationClipDesc Clip;
			std::uint32_t Generation = 1;
		};

		std::vector<Entry> m_entries;
		SceneAnimationSnapshot m_derivedOutput;
	};
}
