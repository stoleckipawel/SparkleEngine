#pragma once

#include "Rendering/RenderObjectId.h"

#include <cstddef>
#include <utility>
#include <vector>

template <typename TLight> class RenderLightCollection final
{
public:
	void Clear() noexcept
	{
		m_lights.clear();
		m_objects.clear();
	}

	void Add(RenderObjectId primitiveId, TLight light)
	{
		m_objects.push_back(primitiveId);
		m_lights.push_back(std::move(light));
	}

	std::size_t size() const noexcept { return m_lights.size(); }
	const TLight& operator[](std::size_t index) const noexcept { return m_lights[index]; }
	RenderObjectId GetObject(std::size_t index) const noexcept { return m_objects[index]; }

private:
	std::vector<TLight> m_lights;
	std::vector<RenderObjectId> m_objects;
};
